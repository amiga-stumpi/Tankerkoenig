#ifndef TANKERKOENIG_HTTPS_H
#define TANKERKOENIG_HTTPS_H
#include <exec/types.h>
#define TK_HTTPS_OK 0
#define TK_HTTPS_NO_LIBRARY -1
#define TK_HTTPS_INIT_FAILED -2
#define TK_HTTPS_BAD_URL -3
#define TK_HTTPS_CONNECT_FAILED -4
#define TK_HTTPS_TLS_FAILED -5
#define TK_HTTPS_WRITE_FAILED -6
#define TK_HTTPS_READ_FAILED -7
#define TK_HTTPS_RESPONSE_TOO_LARGE -8
#define TK_HTTPS_BAD_RESPONSE -9
#define TK_HTTPS_TOO_MANY_REDIRECTS -10
#define TK_HTTPS_UNSUPPORTED_ENCODING -11
typedef struct TKHttpsClient {
    UBYTE library_open;
    UBYTE initialized;
    LONG last_tls_error;
    LONG last_socket_error;
    WORD last_http_status;
    LONG last_response_bytes;
    UBYTE response_chunked;
} TKHttpsClient;
int TK_HttpsOpen(TKHttpsClient *client);
void TK_HttpsClose(TKHttpsClient *client);
int TK_HttpsGet(TKHttpsClient *client, const char *url, UBYTE *output,
    LONG output_size, LONG *output_length);
const char *TK_HttpsErrorText(int error);
#endif
