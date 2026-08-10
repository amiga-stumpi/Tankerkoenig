#include <exec/types.h>
#include <string.h>
#include "tankerkoenig.h"
int main(void)
{
    TKApp app;
    int result = 20;
    memset(&app, 0, sizeof(app));
    TK_LoadConfig(&app.config);
    if (!TK_OpenLibraries()) goto cleanup;
    if (!TK_AllocateBuffers(&app)) goto cleanup;
    if (!TK_OpenWindow(&app)) goto cleanup;
    TK_Draw(&app);
    result = TK_Run(&app);
cleanup:
    TK_CloseWindow(&app);
    TK_SaveConfig(&app.config);
    TK_FreeBuffers(&app);
    TK_CloseLibraries();
    return result;
}
