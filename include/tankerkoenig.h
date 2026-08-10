#ifndef TANKERKOENIG_H
#define TANKERKOENIG_H
#include <exec/types.h>
#include <intuition/intuition.h>
#include "config.h"
#include "https.h"
#include "geocode.h"
#define TK_APP_NAME "Tankerkoenig"
#define TK_VERSION "0.1"
#define TK_MIN_WIDTH 320
#define TK_MIN_HEIGHT 200
#define TK_DEFAULT_WIDTH 480
#define TK_DEFAULT_HEIGHT 240
#define TK_MAX_WIDTH 640
#define TK_MAX_HEIGHT 256
#define TK_NETWORK_BUFFER_SIZE 8192UL
#define TK_JSON_BUFFER_SIZE 32768UL
typedef struct TKApp {
    TKConfig config;
    TKHttpsClient https;
    TKLocationResults locations;
    char status[80];
    struct Window *window;
    UBYTE *network_buffer;
    UBYTE *json_buffer;
    ULONG network_buffer_size;
    ULONG json_buffer_size;
    UBYTE screen_depth;
    WORD selected_location;
} TKApp;
int TK_OpenLibraries(void);
void TK_CloseLibraries(void);
int TK_AllocateBuffers(TKApp *app);
void TK_FreeBuffers(TKApp *app);
int TK_OpenWindow(TKApp *app);
void TK_CloseWindow(TKApp *app);
void TK_Draw(TKApp *app);
int TK_Run(TKApp *app);
void TK_SetStatus(TKApp *app, const char *status);
#endif
