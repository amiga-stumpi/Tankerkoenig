#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <string.h>
#include "tankerkoenig.h"
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
static void draw_text(struct RastPort *rp, WORD x, WORD y, const char *text)
{
    ULONG length = 0;
    while (text[length]) ++length;
    Move(rp, x, y);
    Text(rp, (STRPTR)text, length);
}
static void draw_frame(struct RastPort *rp, WORD l, WORD t, WORD r, WORD b,
    UBYTE bright, UBYTE dark)
{
    SetAPen(rp, bright); Move(rp, l, b); Draw(rp, l, t); Draw(rp, r, t);
    SetAPen(rp, dark); Draw(rp, r, b); Draw(rp, l, b);
}
int TK_OpenLibraries(void)
{
    IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library", 0);
    if (!IntuitionBase) return 0;
    GfxBase = (struct GfxBase *)OpenLibrary((STRPTR)"graphics.library", 0);
    if (!GfxBase) { CloseLibrary((struct Library *)IntuitionBase); IntuitionBase = 0; return 0; }
    return 1;
}
void TK_CloseLibraries(void)
{
    if (GfxBase) { CloseLibrary((struct Library *)GfxBase); GfxBase = 0; }
    if (IntuitionBase) { CloseLibrary((struct Library *)IntuitionBase); IntuitionBase = 0; }
}
int TK_AllocateBuffers(TKApp *app)
{
    if (!app) return 0;
    app->network_buffer_size = TK_NETWORK_BUFFER_SIZE;
    app->json_buffer_size = TK_JSON_BUFFER_SIZE;
    app->network_buffer = (UBYTE *)AllocMem(app->network_buffer_size, MEMF_PUBLIC | MEMF_CLEAR);
    if (!app->network_buffer) return 0;
    app->json_buffer = (UBYTE *)AllocMem(app->json_buffer_size, MEMF_PUBLIC | MEMF_CLEAR);
    if (!app->json_buffer) { FreeMem(app->network_buffer, app->network_buffer_size); app->network_buffer = 0; app->network_buffer_size = 0; return 0; }
    return 1;
}
void TK_FreeBuffers(TKApp *app)
{
    if (!app) return;
    if (app->json_buffer) { FreeMem(app->json_buffer, app->json_buffer_size); app->json_buffer = 0; }
    if (app->network_buffer) { FreeMem(app->network_buffer, app->network_buffer_size); app->network_buffer = 0; }
    app->json_buffer_size = app->network_buffer_size = 0;
}
int TK_OpenWindow(TKApp *app)
{
    struct NewWindow nw;
    if (!app) return 0;
    memset(&nw, 0, sizeof(nw));
    nw.LeftEdge = 0; nw.TopEdge = 0; nw.Width = TK_DEFAULT_WIDTH; nw.Height = TK_DEFAULT_HEIGHT;
    nw.DetailPen = 0; nw.BlockPen = 1;
    nw.IDCMPFlags = IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_NEWSIZE | IDCMP_RAWKEY;
    nw.Flags = WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_SIZEGADGET | WFLG_SMART_REFRESH | WFLG_ACTIVATE;
    nw.Title = (UBYTE *)TK_APP_NAME; nw.MinWidth = TK_MIN_WIDTH; nw.MinHeight = TK_MIN_HEIGHT;
    nw.MaxWidth = TK_MAX_WIDTH; nw.MaxHeight = TK_MAX_HEIGHT; nw.Type = WBENCHSCREEN;
    app->window = OpenWindow(&nw);
    if (!app->window) { nw.Width = TK_MIN_WIDTH; nw.Height = TK_MIN_HEIGHT; app->window = OpenWindow(&nw); }
    if (!app->window) return 0;
    app->screen_depth = app->window->WScreen->BitMap.Depth;
    return 1;
}
void TK_CloseWindow(TKApp *app)
{
    if (app && app->window) { CloseWindow(app->window); app->window = 0; }
}
void TK_Draw(TKApp *app)
{
    struct Window *w; struct RastPort *rp; WORD l, t, r, b; UBYTE bright, dark;
    if (!app || !app->window) return;
    w = app->window; rp = w->RPort;
    l = w->BorderLeft + 8; t = w->BorderTop + 10;
    r = w->Width - w->BorderRight - 8; b = w->Height - w->BorderBottom - 8;
    if (r <= l || b <= t) return;
    bright = app->screen_depth > 1 ? 2 : 1; dark = app->screen_depth > 1 ? 1 : 0;
    SetAPen(rp, 0); RectFill(rp, w->BorderLeft, w->BorderTop, w->Width - w->BorderRight - 1, w->Height - w->BorderBottom - 1);
    draw_frame(rp, l, t, r, b, bright, dark); SetAPen(rp, 1);
    draw_text(rp, l + 10, t + 18, "Tankerkoenig " TK_VERSION);
    draw_text(rp, l + 10, t + 34, "Fuel price finder for AmigaOS");
    draw_text(rp, l + 10, t + 58, "Phase 1: application foundation ready");
}
int TK_Run(TKApp *app)
{
    ULONG mask; int done = 0;
    if (!app || !app->window || !app->window->UserPort) return 20;
    mask = 1UL << app->window->UserPort->mp_SigBit;
    while (!done) {
        struct IntuiMessage *msg;
        Wait(mask);
        while ((msg = (struct IntuiMessage *)GetMsg(app->window->UserPort)) != 0) {
            ULONG cls = msg->Class; UWORD code = msg->Code;
            ReplyMsg((struct Message *)msg);
            if (cls == IDCMP_CLOSEWINDOW) done = 1;
            else if (cls == IDCMP_REFRESHWINDOW) { BeginRefresh(app->window); TK_Draw(app); EndRefresh(app->window, TRUE); }
            else if (cls == IDCMP_NEWSIZE) TK_Draw(app);
            else if (cls == IDCMP_RAWKEY && (code & 0x7f) == 0x10) done = 1;
        }
    }
    return 0;
}
