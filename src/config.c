#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include "config.h"
#define CONFIG_READ_SIZE 2048
#define CONFIG_LINE_SIZE 128
static char g_read_buffer[CONFIG_READ_SIZE];
static char g_line_buffer[CONFIG_LINE_SIZE];
static int ascii_upper(int c) { return c >= 'a' && c <= 'z' ? c - 32 : c; }
static int text_equal_ci(const char *a, const char *b)
{
    while (*a && *b) { if (ascii_upper((UBYTE)*a) != ascii_upper((UBYTE)*b)) return 0; ++a; ++b; }
    return *a == 0 && *b == 0;
}
static void copy_text(char *dst, int size, const char *src)
{
    int i = 0;
    if (size <= 0) return;
    while (src && src[i] && i < size - 1) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}
static char *trim(char *text)
{
    char *end;
    while (*text == ' ' || *text == '\t') ++text;
    end = text + strlen(text);
    while (end > text && (end[-1] == ' ' || end[-1] == '\t')) --end;
    *end = 0;
    return text;
}
static int parse_number(const char *text, long *result)
{
    long value = 0;
    int digits = 0;
    while (*text == ' ' || *text == '\t') ++text;
    while (*text >= '0' && *text <= '9') { if (value > 214748364L) return 0; value = value * 10 + (*text++ - '0'); ++digits; }
    while (*text == ' ' || *text == '\t') ++text;
    if (!digits || *text) return 0;
    *result = value;
    return 1;
}
static int parse_coordinate(const char *text, long limit)
{
    long whole = 0;
    int digits = 0;
    int sign = 1;
    int fraction_digits = 0;
    int fraction_nonzero = 0;
    if (*text == '-') { sign = -1; ++text; } else if (*text == '+') ++text;
    while (*text >= '0' && *text <= '9') { if (whole > 1000) return 0; whole = whole * 10 + (*text++ - '0'); ++digits; }
    if (*text == '.') { ++text; while (*text >= '0' && *text <= '9') { if (*text != '0') fraction_nonzero = 1; ++fraction_digits; ++text; } }
    if (!digits || *text || fraction_digits > 8) return 0;
    if (whole > limit || (whole == limit && fraction_nonzero)) return 0;
    whole *= sign;
    return whole >= -limit && whole <= limit;
}
static int is_hex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static int valid_api_key(const char *key)
{
    int i;
    if (!key[0]) return 1;
    if (strlen(key) != 36) return 0;
    for (i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) { if (key[i] != '-') return 0; }
        else if (!is_hex(key[i])) return 0;
    }
    return 1;
}
const char *TK_ConfigFuelName(WORD fuel)
{
    if (fuel == TK_FUEL_E5) return "e5";
    if (fuel == TK_FUEL_DIESEL) return "diesel";
    if (fuel == TK_FUEL_ALL) return "all";
    return "e10";
}
const char *TK_ConfigSortName(WORD sort) { return sort == TK_SORT_DISTANCE ? "dist" : "price"; }
void TK_ConfigDefaults(TKConfig *config)
{
    if (!config) return;
    memset(config, 0, sizeof(*config));
    copy_text(config->location, sizeof(config->location), "Berlin");
    copy_text(config->latitude, sizeof(config->latitude), "52.5200");
    copy_text(config->longitude, sizeof(config->longitude), "13.4050");
    config->radius = 10; config->fuel = TK_FUEL_E10; config->sort = TK_SORT_PRICE;
    config->open_only = 0; config->update_minutes = 10;
}
static void apply_setting(TKConfig *config, char *line)
{
    char *equals = strchr(line, '=');
    char *key; char *value; long number;
    if (!equals) return;
    *equals = 0; key = trim(line); value = trim(equals + 1);
    if (text_equal_ci(key, "apikey")) { if (valid_api_key(value)) copy_text(config->api_key, sizeof(config->api_key), value); }
    else if (text_equal_ci(key, "location")) { if (value[0]) copy_text(config->location, sizeof(config->location), value); }
    else if (text_equal_ci(key, "latitude")) { if (parse_coordinate(value, 90)) copy_text(config->latitude, sizeof(config->latitude), value); }
    else if (text_equal_ci(key, "longitude")) { if (parse_coordinate(value, 180)) copy_text(config->longitude, sizeof(config->longitude), value); }
    else if (text_equal_ci(key, "radius") && parse_number(value, &number) && number >= 1 && number <= 25) config->radius = (WORD)number;
    else if (text_equal_ci(key, "fuel")) { if (text_equal_ci(value, "e5")) config->fuel = TK_FUEL_E5; else if (text_equal_ci(value, "e10")) config->fuel = TK_FUEL_E10; else if (text_equal_ci(value, "diesel")) config->fuel = TK_FUEL_DIESEL; else if (text_equal_ci(value, "all")) config->fuel = TK_FUEL_ALL; }
    else if (text_equal_ci(key, "sort")) { if (text_equal_ci(value, "price")) config->sort = TK_SORT_PRICE; else if (text_equal_ci(value, "dist") || text_equal_ci(value, "distance")) config->sort = TK_SORT_DISTANCE; }
    else if (text_equal_ci(key, "open_only") && parse_number(value, &number) && (number == 0 || number == 1)) config->open_only = (WORD)number;
    else if (text_equal_ci(key, "update_minutes") && parse_number(value, &number) && number >= 1 && number <= 1440) config->update_minutes = (WORD)number;
}
int TK_LoadConfig(TKConfig *config)
{
    BPTR file; LONG length; LONG i; int position = 0;
    if (!config) return 0;
    TK_ConfigDefaults(config);
    file = Open((STRPTR)TK_CONFIG_FILE, MODE_OLDFILE);
    if (!file) return 0;
    length = Read(file, g_read_buffer, sizeof(g_read_buffer)); Close(file);
    if (length <= 0) return 0;
    for (i = 0; i < length; ++i) {
        char c = g_read_buffer[i];
        if (c == '\r') continue;
        if (c == '\n') { g_line_buffer[position] = 0; if (position && g_line_buffer[0] != '#') apply_setting(config, g_line_buffer); position = 0; }
        else if (position < CONFIG_LINE_SIZE - 1) g_line_buffer[position++] = c;
    }
    if (position) { g_line_buffer[position] = 0; if (g_line_buffer[0] != '#') apply_setting(config, g_line_buffer); }
    return 1;
}
static void write_text(BPTR file, const char *key, const char *value)
{
    Write(file, (APTR)key, strlen(key)); Write(file, "=", 1);
    Write(file, (APTR)value, strlen(value)); Write(file, "\n", 1);
}
static void write_number(BPTR file, const char *key, long value)
{
    char digits[12]; char reverse[12]; int count = 0; int out = 0;
    if (value == 0) reverse[count++] = '0';
    while (value > 0 && count < 12) { reverse[count++] = (char)('0' + value % 10); value /= 10; }
    while (count > 0)
        digits[out++] = reverse[--count];
    digits[out] = 0;
    write_text(file, key, digits);
}
int TK_SaveConfig(const TKConfig *config)
{
    BPTR file;
    if (!config) return 0;
    file = Open((STRPTR)TK_CONFIG_FILE, MODE_NEWFILE); if (!file) return 0;
    Write(file, "# Tankerkoenig user configuration\n", 34);
    write_text(file, "apikey", config->api_key); write_text(file, "location", config->location);
    write_text(file, "latitude", config->latitude); write_text(file, "longitude", config->longitude);
    write_number(file, "radius", config->radius); write_text(file, "fuel", TK_ConfigFuelName(config->fuel));
    write_text(file, "sort", TK_ConfigSortName(config->sort)); write_number(file, "open_only", config->open_only);
    write_number(file, "update_minutes", config->update_minutes); Close(file); return 1;
}
int TK_ConfigHasApiKey(const TKConfig *config) { return config && valid_api_key(config->api_key) && config->api_key[0]; }
