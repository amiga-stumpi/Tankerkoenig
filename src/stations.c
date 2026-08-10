#include <exec/types.h>
#include <string.h>
#include "json.h"
#include "stations.h"
#define STATIONS_URL_SIZE 1024
static char g_url[STATIONS_URL_SIZE];
static void copy(char *dst,long size,const char *src)
{
    long i=0; if (size<=0) return;
    while (src&&src[i]&&i<size-1) { dst[i]=src[i]; ++i; } dst[i]=0;
}
static int append(char *dst,long size,const char *src)
{
    long used=(long)strlen(dst),i=0;
    while (src[i]) { if (used+i>=size-1) return 0; dst[used+i]=src[i]; ++i; }
    dst[used+i]=0; return 1;
}
static int append_number(char *dst,long size,long value)
{
    char reverse[12],digits[12]; int count=0,out=0;
    if (!value) reverse[count++]='0';
    while (value>0&&count<11) { reverse[count++]=(char)('0'+value%10); value/=10; }
    while (count) digits[out++]=reverse[--count];
    digits[out]=0;
    return append(dst,size,digits);
}
static int next_type(TKJsonParser *p,TKJsonToken *t,TKJsonTokenType type)
{
    return TK_JsonNext(p,t)==TK_JSON_OK&&t->type==type;
}
static int text_value(const TKJsonToken *t,char *out,long size)
{
    if (t->type==TK_JSON_TOKEN_NULL) { out[0]=0; return 1; }
    return t->type==TK_JSON_TOKEN_STRING&&TK_JsonDecodeString(t,out,size)==TK_JSON_OK;
}
static int scalar_value(const TKJsonToken *t,char *out,long size)
{
    if (t->type==TK_JSON_TOKEN_FALSE||t->type==TK_JSON_TOKEN_NULL) { out[0]=0; return 1; }
    if (t->type==TK_JSON_TOKEN_STRING) return TK_JsonDecodeString(t,out,size)==TK_JSON_OK;
    if (t->type!=TK_JSON_TOKEN_NUMBER||t->length<=0||t->length>=size) return 0;
    memcpy(out,t->start,t->length); out[t->length]=0; return 1;
}
static int parse_station(TKJsonParser *p,TKStation *station)
{
    TKJsonToken t,key,value; int first=1;
    memset(station,0,sizeof(*station));
    for (;;) {
        if (TK_JsonNext(p,&t)!=TK_JSON_OK) return 0;
        if (t.type==TK_JSON_TOKEN_OBJECT_END) break;
        if (!first) { if (t.type!=TK_JSON_TOKEN_COMMA||TK_JsonNext(p,&t)!=TK_JSON_OK) return 0; }
        if (t.type!=TK_JSON_TOKEN_STRING) return 0;
        key=t;
        if (!next_type(p,&t,TK_JSON_TOKEN_COLON)) return 0;
        if (TK_JsonTokenEquals(&key,"id")||TK_JsonTokenEquals(&key,"name")||TK_JsonTokenEquals(&key,"brand")||
            TK_JsonTokenEquals(&key,"street")||TK_JsonTokenEquals(&key,"houseNumber")||TK_JsonTokenEquals(&key,"postCode")||
            TK_JsonTokenEquals(&key,"place")||TK_JsonTokenEquals(&key,"dist")||TK_JsonTokenEquals(&key,"price")||
            TK_JsonTokenEquals(&key,"e5")||TK_JsonTokenEquals(&key,"e10")||TK_JsonTokenEquals(&key,"diesel")||TK_JsonTokenEquals(&key,"isOpen")) {
            if (TK_JsonNext(p,&value)!=TK_JSON_OK) return 0;
            if (TK_JsonTokenEquals(&key,"id")) { if (!text_value(&value,station->id,sizeof(station->id))) return 0; }
            else if (TK_JsonTokenEquals(&key,"name")) { if (!text_value(&value,station->name,sizeof(station->name))) return 0; }
            else if (TK_JsonTokenEquals(&key,"brand")) { if (!text_value(&value,station->brand,sizeof(station->brand))) return 0; }
            else if (TK_JsonTokenEquals(&key,"street")) { if (!text_value(&value,station->street,sizeof(station->street))) return 0; }
            else if (TK_JsonTokenEquals(&key,"houseNumber")) { if (!scalar_value(&value,station->house_number,sizeof(station->house_number))) return 0; }
            else if (TK_JsonTokenEquals(&key,"postCode")) { if (!scalar_value(&value,station->post_code,sizeof(station->post_code))) return 0; }
            else if (TK_JsonTokenEquals(&key,"place")) { if (!text_value(&value,station->place,sizeof(station->place))) return 0; }
            else if (TK_JsonTokenEquals(&key,"dist")) { if (!scalar_value(&value,station->distance,sizeof(station->distance))) return 0; }
            else if (TK_JsonTokenEquals(&key,"price")) { if (!scalar_value(&value,station->price,sizeof(station->price))) return 0; }
            else if (TK_JsonTokenEquals(&key,"e5")) { if (!scalar_value(&value,station->e5,sizeof(station->e5))) return 0; }
            else if (TK_JsonTokenEquals(&key,"e10")) { if (!scalar_value(&value,station->e10,sizeof(station->e10))) return 0; }
            else if (TK_JsonTokenEquals(&key,"diesel")) { if (!scalar_value(&value,station->diesel,sizeof(station->diesel))) return 0; }
            else if (value.type==TK_JSON_TOKEN_TRUE) station->is_open=1;
            else if (value.type!=TK_JSON_TOKEN_FALSE) return 0;
        } else if (TK_JsonSkipValue(p)!=TK_JSON_OK) return 0;
        first=0;
    }
    return station->name[0]!=0;
}
static int parse_station_array(TKJsonParser *p,const TKConfig *config,TKStationResults *results)
{
    TKJsonToken t; int first=1;
    if (!next_type(p,&t,TK_JSON_TOKEN_ARRAY_BEGIN)) return 0;
    for (;;) {
        TKStation candidate;
        if (TK_JsonNext(p,&t)!=TK_JSON_OK) return 0;
        if (t.type==TK_JSON_TOKEN_ARRAY_END) return 1;
        if (!first) { if (t.type!=TK_JSON_TOKEN_COMMA||TK_JsonNext(p,&t)!=TK_JSON_OK) return 0; }
        if (t.type!=TK_JSON_TOKEN_OBJECT_BEGIN||!parse_station(p,&candidate)) return 0;
        if ((!config->open_only||candidate.is_open)&&results->count<TK_STATION_MAX_RESULTS) results->items[results->count++]=candidate;
        first=0;
    }
}
int TK_StationsDecode(const char *data,LONG length,const TKConfig *config,TKStationResults *results)
{
    TKJsonParser p,check; TKJsonToken t,key,value; int first=1,have_ok=0,ok=0,have_stations=0;
    if (!data||!config||!results) return TK_STATIONS_BAD_RESPONSE;
    TK_JsonInit(&check,data,length); if (TK_JsonValidate(&check)!=TK_JSON_OK) return TK_STATIONS_INVALID_JSON;
    TK_JsonInit(&p,data,length); memset(results,0,sizeof(*results));
    if (!next_type(&p,&t,TK_JSON_TOKEN_OBJECT_BEGIN)) return TK_STATIONS_BAD_RESPONSE;
    for (;;) {
        if (TK_JsonNext(&p,&t)!=TK_JSON_OK) return TK_STATIONS_BAD_RESPONSE;
        if (t.type==TK_JSON_TOKEN_OBJECT_END) break;
        if (!first) { if (t.type!=TK_JSON_TOKEN_COMMA||TK_JsonNext(&p,&t)!=TK_JSON_OK) return TK_STATIONS_BAD_RESPONSE; }
        if (t.type!=TK_JSON_TOKEN_STRING) return TK_STATIONS_BAD_RESPONSE;
        key=t;
        if (!next_type(&p,&t,TK_JSON_TOKEN_COLON)) return TK_STATIONS_BAD_RESPONSE;
        if (TK_JsonTokenEquals(&key,"ok")) {
            if (TK_JsonNext(&p,&value)!=TK_JSON_OK) return TK_STATIONS_BAD_RESPONSE;
            if (value.type==TK_JSON_TOKEN_TRUE) ok=1; else if (value.type!=TK_JSON_TOKEN_FALSE) return TK_STATIONS_BAD_RESPONSE;
            have_ok=1;
        } else if (TK_JsonTokenEquals(&key,"stations")) {
            if (!parse_station_array(&p,config,results)) return TK_STATIONS_BAD_RESPONSE;
            have_stations=1;
        } else if (TK_JsonSkipValue(&p)!=TK_JSON_OK) return TK_STATIONS_BAD_RESPONSE;
        first=0;
    }
    if (!have_ok) return TK_STATIONS_BAD_RESPONSE;
    if (!ok) return TK_STATIONS_API_REJECTED;
    if (!have_stations||!results->count) return TK_STATIONS_NO_RESULTS;
    return TK_STATIONS_OK;
}
int TK_StationsSearch(TKHttpsClient *https,const TKConfig *config,UBYTE *buffer,LONG size,TKStationResults *results)
{
    LONG length=0; int result;
    if (!https||!config||!buffer||size<2||!results) return TK_STATIONS_BAD_RESPONSE;
    if (!TK_ConfigHasApiKey(config)) return TK_STATIONS_MISSING_KEY;
    copy(g_url,sizeof(g_url),"https://creativecommons.tankerkoenig.de/json/list.php?lat=");
    if (!append(g_url,sizeof(g_url),config->latitude)||!append(g_url,sizeof(g_url),"&lng=")||!append(g_url,sizeof(g_url),config->longitude)||
        !append(g_url,sizeof(g_url),"&rad=")||!append_number(g_url,sizeof(g_url),config->radius)||
        !append(g_url,sizeof(g_url),"&type=")||!append(g_url,sizeof(g_url),TK_ConfigFuelName(config->fuel))||
        (config->fuel!=TK_FUEL_ALL&&(!append(g_url,sizeof(g_url),"&sort=")||!append(g_url,sizeof(g_url),TK_ConfigSortName(config->sort))))||
        !append(g_url,sizeof(g_url),"&apikey=")||!append(g_url,sizeof(g_url),config->api_key)) { g_url[0]=0; return TK_STATIONS_BAD_RESPONSE; }
    result=TK_HttpsGet(https,g_url,buffer,size,&length); g_url[0]=0;
    if (result!=TK_HTTPS_OK) return result;
    if (https->last_http_status!=200) return TK_STATIONS_BAD_RESPONSE;
    return TK_StationsDecode((const char *)buffer,length,config,results);
}
const char *TK_StationsErrorText(int error)
{
    if (error==TK_STATIONS_NO_RESULTS) return "No matching fuel stations found";
    if (error==TK_STATIONS_MISSING_KEY) return "Tankerkoenig API key is missing";
    if (error==TK_STATIONS_INVALID_JSON) return "Fuel station response contains invalid JSON";
    if (error==TK_STATIONS_BAD_RESPONSE) return "Invalid fuel station response";
    if (error==TK_STATIONS_API_REJECTED) return "Tankerkoenig API rejected the request";
    return TK_HttpsErrorText(error);
}
