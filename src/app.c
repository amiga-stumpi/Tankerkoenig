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
void TK_SetStatus(TKApp *app,const char *status)
{
    int i=0; if (!app) return;
    while (status&&status[i]&&i<(int)sizeof(app->status)-1) { app->status[i]=status[i]; ++i; }
    app->status[i]=0;
}
static void copy_text(char *dst,int size,const char *src)
{
    int i=0; if (size<=0) return;
    while (src&&src[i]&&i<size-1) { dst[i]=src[i]; ++i; } dst[i]=0;
}
static int append_text(char *dst,int size,const char *src)
{
    int used=(int)strlen(dst),i=0;
    while (src&&src[i]) { if (used+i>=size-1) return 0; dst[used+i]=src[i]; ++i; }
    dst[used+i]=0; return 1;
}
static void append_number(char *dst,int size,LONG value)
{
    char reverse[16],digits[16]; int count=0,out=0; ULONG number;
    if (value<0) { append_text(dst,size,"-"); number=(ULONG)(-(value+1))+1; } else number=(ULONG)value;
    if (!number) reverse[count++]='0';
    while (number&&count<15) { reverse[count++]=(char)('0'+number%10); number/=10; }
    while (count) digits[out++]=reverse[--count];
    digits[out]=0; append_text(dst,size,digits);
}

static void draw_text(struct RastPort *rp,WORD x,WORD y,WORD right,const char *text)
{
    ULONG length=0; while (text[length]) ++length;
    while (length>0&&x+TextLength(rp,(STRPTR)text,length)>right) --length;
    Move(rp,x,y); Text(rp,(STRPTR)text,length);
}
static void draw_frame(struct RastPort *rp,WORD l,WORD t,WORD r,WORD b,UBYTE bright,UBYTE dark)
{
    SetAPen(rp,bright); Move(rp,l,b); Draw(rp,l,t); Draw(rp,r,t);
    SetAPen(rp,dark); Draw(rp,r,b); Draw(rp,l,b);
}
int TK_OpenLibraries(void)
{
    IntuitionBase=(struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library",0); if (!IntuitionBase) return 0;
    GfxBase=(struct GfxBase *)OpenLibrary((STRPTR)"graphics.library",0);
    if (!GfxBase) { CloseLibrary((struct Library *)IntuitionBase); IntuitionBase=0; return 0; }
    return 1;
}
void TK_CloseLibraries(void)
{
    if (GfxBase) { CloseLibrary((struct Library *)GfxBase); GfxBase=0; }
    if (IntuitionBase) { CloseLibrary((struct Library *)IntuitionBase); IntuitionBase=0; }
}
int TK_AllocateBuffers(TKApp *app)
{
    if (!app) return 0;
    app->network_buffer_size=TK_NETWORK_BUFFER_SIZE; app->json_buffer_size=TK_JSON_BUFFER_SIZE;
    app->network_buffer=(UBYTE *)AllocMem(app->network_buffer_size,MEMF_PUBLIC|MEMF_CLEAR); if (!app->network_buffer) return 0;
    app->json_buffer=(UBYTE *)AllocMem(app->json_buffer_size,MEMF_PUBLIC|MEMF_CLEAR);
    if (!app->json_buffer) { FreeMem(app->network_buffer,app->network_buffer_size); app->network_buffer=0; app->network_buffer_size=0; return 0; }
    return 1;
}
void TK_FreeBuffers(TKApp *app)
{
    if (!app) return;
    if (app->json_buffer) { FreeMem(app->json_buffer,app->json_buffer_size); app->json_buffer=0; }
    if (app->network_buffer) { FreeMem(app->network_buffer,app->network_buffer_size); app->network_buffer=0; }
    app->json_buffer_size=app->network_buffer_size=0;
}
int TK_OpenWindow(TKApp *app)
{
    struct NewWindow nw; if (!app) return 0; memset(&nw,0,sizeof(nw));
    nw.LeftEdge=0; nw.TopEdge=0; nw.Width=TK_DEFAULT_WIDTH; nw.Height=TK_DEFAULT_HEIGHT; nw.DetailPen=0; nw.BlockPen=1;
    nw.IDCMPFlags=IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_NEWSIZE|IDCMP_RAWKEY;
    nw.Flags=WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE;
    nw.Title=(UBYTE *)TK_APP_NAME; nw.MinWidth=TK_MIN_WIDTH; nw.MinHeight=TK_MIN_HEIGHT; nw.MaxWidth=TK_MAX_WIDTH; nw.MaxHeight=TK_MAX_HEIGHT; nw.Type=WBENCHSCREEN;
    app->window=OpenWindow(&nw);
    if (!app->window) { nw.LeftEdge=0; nw.TopEdge=0; nw.Width=TK_MIN_WIDTH; nw.Height=TK_MIN_HEIGHT; app->window=OpenWindow(&nw); }
    if (!app->window) return 0;
    app->screen_depth=app->window->WScreen->BitMap.Depth; return 1;
}
void TK_CloseWindow(TKApp *app) { if (app&&app->window) { CloseWindow(app->window); app->window=0; } }
static void location_line(const TKLocationResult *item,int index,char *line,int size)
{
    line[0]=(char)('1'+index); line[1]=':'; line[2]=' '; line[3]=0;
    append_text(line,size,item->name);
    if (item->admin[0]) { append_text(line,size,", "); append_text(line,size,item->admin); }
    if (item->country[0]) { append_text(line,size,", "); append_text(line,size,item->country); }
}
static void station_line(const TKStation *item,const TKConfig *config,char *line,int size)
{
    line[0]=0;
    if (config->fuel==TK_FUEL_ALL) {
        append_text(line,size,"D:"); append_text(line,size,item->diesel[0]?item->diesel:"-");
        append_text(line,size," E5:"); append_text(line,size,item->e5[0]?item->e5:"-");
        append_text(line,size," E10:"); append_text(line,size,item->e10[0]?item->e10:"-");
    } else {
        append_text(line,size,item->price[0]?item->price:"-"); append_text(line,size," EUR");
    }
    append_text(line,size,"  "); append_text(line,size,item->name);
    if (item->distance[0]) { append_text(line,size," ("); append_text(line,size,item->distance); append_text(line,size," km)"); }
    append_text(line,size,item->is_open?" open":" closed");
}
void TK_Draw(TKApp *app)
{
    struct Window *w; struct RastPort *rp; WORD l,t,r,b; UBYTE bright,dark; int i; char line[160];
    if (!app||!app->window) return;
    w=app->window; rp=w->RPort;
    l=w->BorderLeft+8; t=w->BorderTop+8; r=w->Width-w->BorderRight-8; b=w->Height-w->BorderBottom-8; if (r<=l||b<=t) return;
    bright=app->screen_depth>1?2:1; dark=app->screen_depth>1?1:0;
    SetAPen(rp,0); RectFill(rp,w->BorderLeft,w->BorderTop,w->Width-w->BorderRight-1,w->Height-w->BorderBottom-1);
    draw_frame(rp,l,t,r,b,bright,dark); SetAPen(rp,1);
    draw_text(rp,l+10,t+16,r-4,"Tankerkoenig " TK_VERSION " - Fuel price finder");
    draw_text(rp,l+10,t+34,r-4,"Location:"); draw_text(rp,l+90,t+34,r-4,app->config.location);
    draw_text(rp,l+10,t+48,r-4,"Fuel:"); draw_text(rp,l+90,t+48,r-4,TK_ConfigFuelName(app->config.fuel));
    draw_text(rp,l+10,t+62,r-4,"Sort:"); draw_text(rp,l+90,t+62,r-4,TK_ConfigSortName(app->config.sort));
    draw_text(rp,l+10,t+76,r-4,"API key:"); draw_text(rp,l+90,t+76,r-4,TK_ConfigHasApiKey(&app->config)?"configured":"missing");
    draw_text(rp,l+10,t+94,r-4,app->status);
    if (app->stations.count) {
        draw_text(rp,l+10,t+112,r-4,"Nearby fuel stations:");
        for (i=0;i<app->stations.count&&i<4;++i) {
            station_line(&app->stations.items[i],&app->config,line,sizeof(line));
            draw_text(rp,l+10,t+128+i*14,r-4,line);
        }
    } else if (app->locations.count) {
        draw_text(rp,l+10,t+112,r-4,"Select location with keys 1-4:");
        for (i=0;i<app->locations.count;++i) {
            location_line(&app->locations.items[i],i,line,sizeof(line));
            SetAPen(rp,i==app->selected_location?bright:1); draw_text(rp,l+10,t+128+i*14,r-4,line);
        }
        SetAPen(rp,1);
    }
}
static void search_locations(TKApp *app)
{
    int result;
    if (!app->https.initialized) { TK_SetStatus(app,"HTTPS is not available"); TK_Draw(app); return; }
    app->locations.count=0; app->stations.count=0; app->selected_location=-1; TK_SetStatus(app,"Searching locations - please wait"); TK_Draw(app);
    result=TK_GeocodeSearch(&app->https,app->config.location,app->json_buffer,app->json_buffer_size,&app->locations);
    if (result==TK_GEOCODE_OK) TK_SetStatus(app,"Location results loaded");
    else TK_SetStatus(app,TK_GeocodeErrorText(result));
    TK_Draw(app);
}
static void select_location(TKApp *app,int index)
{
    TKLocationResult *item;
    if (index<0||index>=app->locations.count) return;
    item=&app->locations.items[index]; app->selected_location=(WORD)index;
    copy_text(app->config.location,sizeof(app->config.location),item->name);
    copy_text(app->config.latitude,sizeof(app->config.latitude),item->latitude);
    copy_text(app->config.longitude,sizeof(app->config.longitude),item->longitude);
    if (TK_SaveConfig(&app->config)) TK_SetStatus(app,"Location selected and saved"); else TK_SetStatus(app,"Location selected; configuration save failed");
    TK_Draw(app);
}
static void update_stations(TKApp *app)
{
    int result;
    if (!app->https.initialized) { TK_SetStatus(app,"HTTPS is not available"); TK_Draw(app); return; }
    app->stations.count=0; TK_SetStatus(app,"Loading fuel prices - please wait"); TK_Draw(app);
    result=TK_StationsSearch(&app->https,&app->config,app->json_buffer,app->json_buffer_size,&app->stations);
    if (result==TK_STATIONS_OK) { app->locations.count=0; TK_SetStatus(app,"Fuel prices loaded"); }
    else if (result==TK_HTTPS_READ_FAILED) {
        char detail[80]; detail[0]=0; append_text(detail,sizeof(detail),"Read fail B="); append_number(detail,sizeof(detail),app->https.last_response_bytes);
        append_text(detail,sizeof(detail)," TLS="); append_number(detail,sizeof(detail),app->https.last_tls_error);
        append_text(detail,sizeof(detail)," SE="); append_number(detail,sizeof(detail),app->https.last_socket_error);
        append_text(detail,sizeof(detail)," C="); append_number(detail,sizeof(detail),app->https.response_chunked); TK_SetStatus(app,detail);
    } else TK_SetStatus(app,TK_StationsErrorText(result));
    TK_Draw(app);
}
int TK_Run(TKApp *app)
{
    ULONG mask; int done=0; if (!app||!app->window||!app->window->UserPort) return 20; mask=1UL<<app->window->UserPort->mp_SigBit;
    while (!done) {
        struct IntuiMessage *msg; Wait(mask);
        while ((msg=(struct IntuiMessage *)GetMsg(app->window->UserPort))!=0) {
            ULONG cls=msg->Class; UWORD code=msg->Code; ReplyMsg((struct Message *)msg);
            if (cls==IDCMP_CLOSEWINDOW) done=1;
            else if (cls==IDCMP_REFRESHWINDOW) { BeginRefresh(app->window); TK_Draw(app); EndRefresh(app->window,TRUE); }
            else if (cls==IDCMP_NEWSIZE) TK_Draw(app);
            else if (cls==IDCMP_RAWKEY&&!(code&0x80)&&(code&0x7f)==0x10) done=1;
            else if (cls==IDCMP_RAWKEY&&!(code&0x80)&&(code&0x7f)==0x21) search_locations(app);
            else if (cls==IDCMP_RAWKEY&&!(code&0x80)&&(code&0x7f)==0x16) update_stations(app);
            else if (cls==IDCMP_RAWKEY&&!(code&0x80)&&(code&0x7f)>=0x01&&(code&0x7f)<=0x04) select_location(app,(code&0x7f)-1);
        }
    }
    return 0;
}
