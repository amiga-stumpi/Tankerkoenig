#include <exec/types.h>
#include <string.h>
#include "geocode.h"
#include "json.h"
#define GEOCODE_URL_SIZE 768
static char g_url[GEOCODE_URL_SIZE];
static const char g_hex[]="0123456789ABCDEF";
static void copy(char *dst,long size,const char *src)
{
    long i=0; if (size<=0) return;
    while (src && src[i] && i<size-1) { dst[i]=src[i]; ++i; } dst[i]=0;
}
static int append(char *dst,long size,const char *src)
{
    long used=(long)strlen(dst),i=0;
    while (src[i]) { if (used+i>=size-1) return 0; dst[used+i]=src[i]; ++i; }
    dst[used+i]=0; return 1;
}
static int append_encoded_byte(char *dst,long size,long *used,unsigned int c)
{
    if (*used+3>=size) return 0;
    dst[(*used)++]='%'; dst[(*used)++]=g_hex[(c>>4)&15]; dst[(*used)++]=g_hex[c&15]; return 1;
}
static int url_encode_location(const char *text,char *out,long size)
{
    long used=0; unsigned int c;
    if (!text || !text[0] || size<2) return 0;
    while (*text) {
        c=(unsigned char)*text++;
        if ((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~') {
            if (used>=size-1) return 0;
            out[used++]=(char)c;
        } else if (c==' ') {
            if (used>=size-1) return 0;
            out[used++]='+';
        } else if (c>=0x80) {
            unsigned int first=0xC0|(c>>6),second=0x80|(c&0x3F);
            if (!append_encoded_byte(out,size,&used,first)||!append_encoded_byte(out,size,&used,second)) return 0;
        } else if (!append_encoded_byte(out,size,&used,c)) return 0;
    }
    out[used]=0; return 1;
}
static int next_type(TKJsonParser *p,TKJsonToken *t,TKJsonTokenType type)
{
    return TK_JsonNext(p,t)==TK_JSON_OK && t->type==type;
}
static int token_text(const TKJsonToken *t,char *out,long size)
{
    if (t->type!=TK_JSON_TOKEN_STRING) return 0;
    return TK_JsonDecodeString(t,out,size)==TK_JSON_OK;
}
static int token_number(const TKJsonToken *t,char *out,long size)
{
    if (t->type!=TK_JSON_TOKEN_NUMBER || t->length<=0 || t->length>=size) return 0;
    memcpy(out,t->start,t->length); out[t->length]=0; return 1;
}
static int parse_result_object(TKJsonParser *p,TKLocationResult *item)
{
    TKJsonToken t,key,value; int first=1;
    memset(item,0,sizeof(*item));
    for (;;) {
        if (!next_type(p,&t,first?TK_JSON_TOKEN_STRING:TK_JSON_TOKEN_COMMA)) {
            if (t.type==TK_JSON_TOKEN_OBJECT_END) break;
            return 0;
        }
        if (!first) {
            if (TK_JsonNext(p,&t)!=TK_JSON_OK) return 0;
            if (t.type==TK_JSON_TOKEN_OBJECT_END) return 0;
            if (t.type!=TK_JSON_TOKEN_STRING) return 0;
        }
        key=t;
        if (!next_type(p,&t,TK_JSON_TOKEN_COLON)) return 0;
        if (TK_JsonTokenEquals(&key,"name")||TK_JsonTokenEquals(&key,"admin1")||
            TK_JsonTokenEquals(&key,"country")||TK_JsonTokenEquals(&key,"latitude")||
            TK_JsonTokenEquals(&key,"longitude")) {
            if (TK_JsonNext(p,&value)!=TK_JSON_OK) return 0;
            if (TK_JsonTokenEquals(&key,"name")) { if (!token_text(&value,item->name,sizeof(item->name))) return 0; }
            else if (TK_JsonTokenEquals(&key,"admin1")) { if (!token_text(&value,item->admin,sizeof(item->admin))) return 0; }
            else if (TK_JsonTokenEquals(&key,"country")) { if (!token_text(&value,item->country,sizeof(item->country))) return 0; }
            else if (TK_JsonTokenEquals(&key,"latitude")) { if (!token_number(&value,item->latitude,sizeof(item->latitude))) return 0; }
            else if (!token_number(&value,item->longitude,sizeof(item->longitude))) return 0;
        } else if (TK_JsonSkipValue(p)!=TK_JSON_OK) return 0;
        first=0;
        {
            long saved=p->position; int depth=p->depth;
            if (TK_JsonNext(p,&t)!=TK_JSON_OK) return 0;
            if (t.type==TK_JSON_TOKEN_OBJECT_END) break;
            p->position=saved; p->depth=depth;
        }
    }
    return item->name[0]&&item->latitude[0]&&item->longitude[0];
}
static int parse_results(TKJsonParser *p,TKLocationResults *results)
{
    TKJsonToken t; int first=1;
    if (!next_type(p,&t,TK_JSON_TOKEN_ARRAY_BEGIN)) return 0;
    for (;;) {
        if (TK_JsonNext(p,&t)!=TK_JSON_OK) return 0;
        if (t.type==TK_JSON_TOKEN_ARRAY_END) return 1;
        if (!first) {
            if (t.type!=TK_JSON_TOKEN_COMMA || TK_JsonNext(p,&t)!=TK_JSON_OK) return 0;
        }
        if (t.type!=TK_JSON_TOKEN_OBJECT_BEGIN) return 0;
        if (results->count<TK_GEOCODE_MAX_RESULTS) {
            if (!parse_result_object(p,&results->items[results->count])) return 0;
            ++results->count;
        } else { TKLocationResult ignored; if (!parse_result_object(p,&ignored)) return 0; }
        first=0;
    }
}
int TK_GeocodeDecode(const char *data,LONG length,TKLocationResults *results)
{
    TKJsonParser p,check; TKJsonToken t,key; int first=1,found=0;
    TK_JsonInit(&check,data,length); if (TK_JsonValidate(&check)!=TK_JSON_OK) return TK_GEOCODE_INVALID_JSON;
    TK_JsonInit(&p,data,length); memset(results,0,sizeof(*results));
    if (!next_type(&p,&t,TK_JSON_TOKEN_OBJECT_BEGIN)) return TK_GEOCODE_BAD_RESPONSE;
    for (;;) {
        if (TK_JsonNext(&p,&t)!=TK_JSON_OK) return TK_GEOCODE_BAD_RESPONSE;
        if (t.type==TK_JSON_TOKEN_OBJECT_END) break;
        if (!first) {
            if (t.type!=TK_JSON_TOKEN_COMMA || TK_JsonNext(&p,&t)!=TK_JSON_OK) return TK_GEOCODE_BAD_RESPONSE;
        }
        if (t.type!=TK_JSON_TOKEN_STRING) return TK_GEOCODE_BAD_RESPONSE;
        key=t; if (!next_type(&p,&t,TK_JSON_TOKEN_COLON)) return TK_GEOCODE_BAD_RESPONSE;
        if (TK_JsonTokenEquals(&key,"results")) { if (!parse_results(&p,results)) return TK_GEOCODE_BAD_RESPONSE; found=1; }
        else if (TK_JsonSkipValue(&p)!=TK_JSON_OK) return TK_GEOCODE_BAD_RESPONSE;
        first=0;
    }
    if (!found || !results->count) return TK_GEOCODE_NO_RESULTS;
    return TK_GEOCODE_OK;
}
int TK_GeocodeSearch(TKHttpsClient *https,const char *location,UBYTE *buffer,LONG size,TKLocationResults *results)
{
    char encoded[256]; LONG length=0; int result;
    if (!https||!results||!buffer||size<2||!url_encode_location(location,encoded,sizeof(encoded))) return TK_GEOCODE_BAD_QUERY;
    copy(g_url,sizeof(g_url),"https://geocoding-api.open-meteo.com/v1/search?name=");
    if (!append(g_url,sizeof(g_url),encoded)||!append(g_url,sizeof(g_url),"&count=10&language=en&format=json")) return TK_GEOCODE_BAD_QUERY;
    result=TK_HttpsGet(https,g_url,buffer,size,&length);
    if (result!=TK_HTTPS_OK) return result;
    if (https->last_http_status!=200) return TK_GEOCODE_BAD_RESPONSE;
    return TK_GeocodeDecode((const char *)buffer,length,results);
}
const char *TK_GeocodeErrorText(int error)
{
    if (error==TK_GEOCODE_NO_RESULTS) return "No matching locations found";
    if (error==TK_GEOCODE_BAD_QUERY) return "Location search text is invalid";
    if (error==TK_GEOCODE_INVALID_JSON) return "Location response contains invalid JSON";
    if (error==TK_GEOCODE_BAD_RESPONSE) return "Invalid location response";
    return TK_HttpsErrorText(error);
}
