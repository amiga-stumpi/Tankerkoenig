#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <string.h>
#include "amitls13.h"
#include "amitls13_libbase.h"
#include "https.h"
#define HTTPS_HOST_SIZE 128
#define HTTPS_PATH_SIZE 512
#define HTTPS_URL_SIZE 768
#define HTTPS_REQUEST_SIZE 768
#define HTTPS_LOCATION_SIZE 512
#define HTTPS_REDIRECT_LIMIT 3
struct Library *AmiTLS13Base = 0;
static char g_host[HTTPS_HOST_SIZE];
static char g_path[HTTPS_PATH_SIZE];
static char g_request[HTTPS_REQUEST_SIZE];
static char g_location[HTTPS_LOCATION_SIZE];
static char g_current_url[HTTPS_URL_SIZE];
static int starts_with(const char *text, const char *prefix)
{
    while (*prefix) if (*text++ != *prefix++) return 0;
    return 1;
}
static int ascii_upper(int c) { return c >= 'a' && c <= 'z' ? c - 32 : c; }
static int prefix_ci(const char *text, const char *prefix)
{
    while (*prefix) if (ascii_upper((UBYTE)*text++) != ascii_upper((UBYTE)*prefix++)) return 0;
    return 1;
}
static void copy_text(char *dst, LONG size, const char *src)
{
    LONG i = 0;
    if (size <= 0) return;
    while (src && src[i] && i < size - 1) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}
static int append_text(char *dst, LONG size, const char *src)
{
    LONG used = strlen(dst); LONG i = 0;
    while (src[i]) { if (used + i >= size - 1) return 0; dst[used + i] = src[i]; ++i; }
    dst[used + i] = 0; return 1;
}
static UWORD parse_port(const char *text, const char **end)
{
    ULONG value = 0; int digits = 0;
    while (*text >= '0' && *text <= '9') { value = value * 10UL + (ULONG)(*text++ - '0'); ++digits; if (value > 65535UL) break; }
    *end = text; return digits && value > 0 && value <= 65535UL ? (UWORD)value : 0;
}
static int parse_url(const char *url, char *host, LONG host_size,
    char *path, LONG path_size, UWORD *port)
{
    const char *start; const char *slash; const char *colon; const char *port_end; LONG length;
    if (!url || !starts_with(url, "https://")) return 0;
    start = url + 8; slash = start; while (*slash && *slash != '/') ++slash;
    colon = start; while (colon < slash && *colon != ':') ++colon;
    length = colon < slash ? (LONG)(colon - start) : (LONG)(slash - start);
    if (length <= 0 || length >= host_size) return 0;
    memcpy(host, start, length); host[length] = 0; *port = 443;
    if (colon < slash) { *port = parse_port(colon + 1, &port_end); if (!*port || port_end != slash) return 0; }
    if (*slash) { if ((LONG)strlen(slash) >= path_size) return 0; copy_text(path, path_size, slash); }
    else copy_text(path, path_size, "/");
    return 1;
}
static char *find_header_end(char *data, LONG length, LONG *header_size)
{
    LONG i;
    for (i = 0; i + 3 < length; ++i) if (data[i] == '\r' && data[i+1] == '\n' && data[i+2] == '\r' && data[i+3] == '\n') { *header_size = i + 4; return data + i + 4; }
    for (i = 0; i + 1 < length; ++i) if (data[i] == '\n' && data[i+1] == '\n') { *header_size = i + 2; return data + i + 2; }
    return 0;
}
static WORD parse_status(const char *data, LONG length)
{
    LONG i = 0; LONG value = 0; int digits = 0;
    if (length < 9 || !starts_with(data, "HTTP/")) return 0;
    while (i < length && data[i] != ' ' && data[i] != '\n') ++i;
    while (i < length && data[i] == ' ') ++i;
    while (i < length && data[i] >= '0' && data[i] <= '9') { value = value * 10 + data[i++] - '0'; ++digits; }
    return digits == 3 ? (WORD)value : 0;
}
static LONG parse_content_length(const char *data, LONG header_size)
{
    LONG i = 0;
    while (i < header_size) {
        LONG start = i; LONG value = 0; int digits = 0;
        while (i < header_size && data[i] != '\n') ++i;
        if (prefix_ci(data + start, "Content-Length:")) {
            start += 15; while (start < i && (data[start] == ' ' || data[start] == '\t')) ++start;
            while (start < i && data[start] >= '0' && data[start] <= '9') { value = value * 10 + data[start++] - '0'; ++digits; }
            return digits ? value : -1;
        }
        ++i;
    }
    return -1;
}
static int header_is_chunked(const char *data, LONG header_size)
{
    LONG i = 0;
    while (i < header_size) {
        LONG start = i;
        while (i < header_size && data[i] != '\n') ++i;
        if (prefix_ci(data + start, "Transfer-Encoding:")) {
            while (start < i) { if (prefix_ci(data + start, "chunked")) return 1; ++start; }
        }
        ++i;
    }
    return 0;
}
static int extract_location(const char *data, LONG header_size, char *out, LONG out_size)
{
    LONG i = 0;
    while (i < header_size) {
        LONG start = i; LONG end;
        while (i < header_size && data[i] != '\n') ++i;
        end = i; if (end > start && data[end-1] == '\r') --end;
        if (prefix_ci(data + start, "Location:")) {
            start += 9; while (start < end && (data[start] == ' ' || data[start] == '\t')) ++start;
            if (end - start <= 0 || end - start >= out_size) return 0;
            memcpy(out, data + start, end - start); out[end-start] = 0; return 1;
        }
        ++i;
    }
    return 0;
}
static int make_redirect_url(const char *old_url, const char *location,
    char *out, LONG out_size)
{
    const char *host_start; const char *path; LONG prefix_length;
    if (starts_with(location, "https://")) { if ((LONG)strlen(location) >= out_size) return 0; copy_text(out, out_size, location); return 1; }
    if (location[0] != '/' || !starts_with(old_url, "https://")) return 0;
    host_start = old_url + 8; path = host_start; while (*path && *path != '/') ++path;
    prefix_length = (LONG)(path - old_url);
    if (prefix_length + (LONG)strlen(location) >= out_size) return 0;
    memcpy(out, old_url, prefix_length); out[prefix_length] = 0; return append_text(out, out_size, location);
}
int TK_HttpsOpen(TKHttpsClient *client)
{
    LONG result;
    if (!client) return TK_HTTPS_INIT_FAILED;
    memset(client, 0, sizeof(*client));
    AmiTLS13Base = OpenLibrary((STRPTR)AMITLS13NAME, AMITLS13VERSION);
    if (!AmiTLS13Base) return TK_HTTPS_NO_LIBRARY;
    client->library_open = 1;
    result = AmiTLS13_Init();
    if (result != AMITLS13_OK) { client->last_socket_error = AmiTLS13_SocketErrno(); TK_HttpsClose(client); return TK_HTTPS_INIT_FAILED; }
    client->initialized = 1; return TK_HTTPS_OK;
}
void TK_HttpsClose(TKHttpsClient *client)
{
    if (!client) return;
    if (client->initialized) { AmiTLS13_Exit(); client->initialized = 0; }
    if (client->library_open && AmiTLS13Base) { CloseLibrary(AmiTLS13Base); AmiTLS13Base = 0; client->library_open = 0; }
}
static int request_once(TKHttpsClient *client, const char *url, UBYTE *output,
    LONG output_size, LONG *output_length, WORD *status, char *redirect_location)
{
    struct AmiTLS13Context *context = 0; UWORD port; LONG written; LONG used = 0; LONG received;
    LONG header_size; LONG body_length; LONG declared_length; LONG expected_total = -1;
    LONG early_header_size = 0; char *body; char *early_body; UBYTE extra;
    int result = TK_HTTPS_OK;
    redirect_location[0] = 0;
    if (!parse_url(url, g_host, sizeof(g_host), g_path, sizeof(g_path), &port)) return TK_HTTPS_BAD_URL;
    context = AmiTLS13_Connect(g_host, port, AMITLS13F_INSECURE);
    if (!context) { client->last_socket_error = AmiTLS13_SocketErrno(); return TK_HTTPS_CONNECT_FAILED; }
    if (AmiTLS13_StartTLS(context, g_host) != AMITLS13_OK) { client->last_tls_error = AmiTLS13_GetLastError(context); result = TK_HTTPS_TLS_FAILED; goto done; }
    g_request[0] = 0;
    if (!append_text(g_request, sizeof(g_request), "GET ") || !append_text(g_request, sizeof(g_request), g_path) ||
        !append_text(g_request, sizeof(g_request), " HTTP/1.0\r\nHost: ") || !append_text(g_request, sizeof(g_request), g_host) ||
        !append_text(g_request, sizeof(g_request), "\r\nUser-Agent: Tankerkoenig/0.1 (AmigaOS)\r\nAccept: application/json\r\nConnection: close\r\n\r\n")) { result = TK_HTTPS_BAD_URL; goto done; }
    written = AmiTLS13_Write(context, (const UBYTE *)g_request, strlen(g_request));
    if (written != (LONG)strlen(g_request)) { client->last_tls_error = AmiTLS13_GetLastError(context); result = TK_HTTPS_WRITE_FAILED; goto done; }
    while (used < output_size - 1) {
        received = AmiTLS13_Read(context, output + used, output_size - 1 - used);
        if (received < 0) {
            if (expected_total >= 0 && used >= expected_total) break;
            client->last_tls_error = AmiTLS13_GetLastError(context); result = TK_HTTPS_READ_FAILED; goto done;
        }
        if (received == 0) break;
        used += received;
        output[used] = 0;
        if (expected_total < 0) {
            early_body = find_header_end((char *)output, used, &early_header_size);
            if (early_body) {
                declared_length = parse_content_length((char *)output, early_header_size);
                if (declared_length >= 0) {
                    if (declared_length > output_size - 1 - early_header_size) { result = TK_HTTPS_RESPONSE_TOO_LARGE; goto done; }
                    expected_total = early_header_size + declared_length;
                }
            }
        }
        if (expected_total >= 0 && used >= expected_total) break;
    }
    if (used == output_size - 1 && (expected_total < 0 || used < expected_total)) { received = AmiTLS13_Read(context, &extra, 1); if (received > 0) { result = TK_HTTPS_RESPONSE_TOO_LARGE; goto done; } if (received < 0) { result = TK_HTTPS_READ_FAILED; goto done; } }
    output[used] = 0; *status = parse_status((char *)output, used);
    if (!*status) { result = TK_HTTPS_BAD_RESPONSE; goto done; }
    body = find_header_end((char *)output, used, &header_size);
    if (!body) { result = TK_HTTPS_BAD_RESPONSE; goto done; }
    if ((*status == 301 || *status == 302 || *status == 303 || *status == 307 || *status == 308) &&
        !extract_location((char *)output, header_size, redirect_location, HTTPS_LOCATION_SIZE)) { result = TK_HTTPS_BAD_RESPONSE; goto done; }
    if (header_is_chunked((char *)output, header_size)) { result = TK_HTTPS_UNSUPPORTED_ENCODING; goto done; }
    body_length = used - header_size; declared_length = parse_content_length((char *)output, header_size);
    if (declared_length >= 0 && body_length < declared_length) { result = TK_HTTPS_BAD_RESPONSE; goto done; }
    if (declared_length >= 0) body_length = declared_length;
    memmove(output, body, body_length); output[body_length] = 0; *output_length = body_length;
done:
    if (context) AmiTLS13_Close(context);
    return result;
}
int TK_HttpsGet(TKHttpsClient *client, const char *url, UBYTE *output,
    LONG output_size, LONG *output_length)
{
    int redirect; int result; WORD status = 0;
    if (output_length) *output_length = 0;
    if (!client || !client->initialized) return TK_HTTPS_INIT_FAILED;
    if (!url || !output || output_size < 2 || !output_length) return TK_HTTPS_BAD_URL;
    if ((LONG)strlen(url) >= (LONG)sizeof(g_current_url)) return TK_HTTPS_BAD_URL;
    copy_text(g_current_url, sizeof(g_current_url), url);
    for (redirect = 0; redirect <= HTTPS_REDIRECT_LIMIT; ++redirect) {
        result = request_once(client, g_current_url, output, output_size, output_length, &status, g_location);
        client->last_http_status = status;
        if (result != TK_HTTPS_OK) return result;
        if (status != 301 && status != 302 && status != 303 && status != 307 && status != 308) return TK_HTTPS_OK;
        if (redirect == HTTPS_REDIRECT_LIMIT) return TK_HTTPS_TOO_MANY_REDIRECTS;
        if (!make_redirect_url(g_current_url, g_location, g_path, sizeof(g_path))) return TK_HTTPS_BAD_RESPONSE;
        copy_text(g_current_url, sizeof(g_current_url), g_path);
    }
    return TK_HTTPS_TOO_MANY_REDIRECTS;
}
const char *TK_HttpsErrorText(int error)
{
    if (error == TK_HTTPS_OK) return "HTTPS request complete";
    if (error == TK_HTTPS_NO_LIBRARY) return "AmiTLS13 2.0 not installed";
    if (error == TK_HTTPS_INIT_FAILED) return "TCP/IP stack not available";
    if (error == TK_HTTPS_BAD_URL) return "Invalid HTTPS URL";
    if (error == TK_HTTPS_CONNECT_FAILED) return "HTTPS connection failed";
    if (error == TK_HTTPS_TLS_FAILED) return "TLS handshake failed";
    if (error == TK_HTTPS_WRITE_FAILED) return "HTTP request send failed";
    if (error == TK_HTTPS_READ_FAILED) return "HTTP response read failed";
    if (error == TK_HTTPS_RESPONSE_TOO_LARGE) return "HTTP response too large";
    if (error == TK_HTTPS_TOO_MANY_REDIRECTS) return "Too many HTTP redirects";
    if (error == TK_HTTPS_UNSUPPORTED_ENCODING) return "Unsupported HTTP encoding";
    return "Invalid HTTP response";
}
