#include <exec/types.h>
#include <string.h>
#include "tankerkoenig.h"
int main(void)
{
    TKApp app;
    int result = 20;
    memset(&app, 0, sizeof(app));
    TK_LoadConfig(&app.config);
    app.selected_location = -1;
    if (!TK_OpenLibraries()) goto cleanup;
    if (!TK_AllocateBuffers(&app)) goto cleanup;
    {
        int https_result = TK_HttpsOpen(&app.https);
        TK_SetStatus(&app, https_result == TK_HTTPS_OK ? "HTTPS ready - press S to search" : TK_HttpsErrorText(https_result));
    }
    if (!TK_OpenWindow(&app)) goto cleanup;
    TK_Draw(&app);
    result = TK_Run(&app);
cleanup:
    TK_CloseWindow(&app);
    TK_HttpsClose(&app.https);
    TK_SaveConfig(&app.config);
    TK_FreeBuffers(&app);
    TK_CloseLibraries();
    return result;
}
