#include <string.h>
#include "json.h"

static int space(int c) { return c==' ' || c=='\t' || c=='\r' || c=='\n'; }
static int digit(int c) { return c>='0' && c<='9'; }
static int hex(int c)
{
    if (c>='0' && c<='9') return c-'0';
    if (c>='a' && c<='f') return c-'a'+10;
    if (c>='A' && c<='F') return c-'A'+10;
    return -1;
}
static int fail(TKJsonParser *p, int e) { p->error=e; return e; }
static int word(TKJsonParser *p,TKJsonToken *t,const char *s,TKJsonTokenType type)
{
    long n=(long)strlen(s);
    if (p->position+n>p->length) return fail(p,TK_JSON_INCOMPLETE);
    if (memcmp(p->data+p->position,s,n)) return fail(p,TK_JSON_ERROR);
    t->type=type; t->start=p->data+p->position; t->length=n; p->position+=n;
    return TK_JSON_OK;
}
void TK_JsonInit(TKJsonParser *p,const char *data,long length)
{
    if (!p) return;
    p->data=data; p->length=length>=0?length:0; p->position=0; p->depth=0;
    p->error=data?TK_JSON_OK:TK_JSON_ERROR;
}
int TK_JsonNext(TKJsonParser *p,TKJsonToken *t)
{
    long start; int c;
    if (!p || !t || !p->data) return TK_JSON_ERROR;
    if (p->error<0) return p->error;
    while (p->position<p->length && space((unsigned char)p->data[p->position])) ++p->position;
    t->type=TK_JSON_TOKEN_NONE; t->start=0; t->length=0;
    if (p->position>=p->length) return TK_JSON_END;
    start=p->position; c=(unsigned char)p->data[p->position++];
    if (c=='{' || c=='[') {
        if (p->depth>=TK_JSON_MAX_DEPTH) return fail(p,TK_JSON_DEPTH_EXCEEDED);
        ++p->depth; t->type=c=='{'?TK_JSON_TOKEN_OBJECT_BEGIN:TK_JSON_TOKEN_ARRAY_BEGIN;
    } else if (c=='}' || c==']') {
        if (p->depth<=0) return fail(p,TK_JSON_ERROR);
        --p->depth; t->type=c=='}'?TK_JSON_TOKEN_OBJECT_END:TK_JSON_TOKEN_ARRAY_END;
    } else if (c==':') t->type=TK_JSON_TOKEN_COLON;
    else if (c==',') t->type=TK_JSON_TOKEN_COMMA;
    else if (c=='"') {
        start=p->position;
        while (p->position<p->length) {
            c=(unsigned char)p->data[p->position++];
            if (c=='"') {
                t->type=TK_JSON_TOKEN_STRING; t->start=p->data+start;
                t->length=p->position-start-1;
                if (t->length>TK_JSON_MAX_STRING) return fail(p,TK_JSON_STRING_TOO_LONG);
                return TK_JSON_OK;
            }
            if (c<0x20) return fail(p,TK_JSON_ERROR);
            if (c=='\\') {
                int i;
                if (p->position>=p->length) return fail(p,TK_JSON_INCOMPLETE);
                c=(unsigned char)p->data[p->position++];
                if (!strchr("\"\\/bfnrtu",c)) return fail(p,TK_JSON_ERROR);
                if (c=='u') {
                    if (p->position+4>p->length) return fail(p,TK_JSON_INCOMPLETE);
                    for (i=0;i<4;++i) if (hex((unsigned char)p->data[p->position+i])<0) return fail(p,TK_JSON_ERROR);
                    p->position+=4;
                }
            }
        }
        return fail(p,TK_JSON_INCOMPLETE);
    } else if (c=='-' || digit(c)) {
        if (c=='-') {
            if (p->position>=p->length) return fail(p,TK_JSON_INCOMPLETE);
            c=(unsigned char)p->data[p->position++];
            if (!digit(c)) return fail(p,TK_JSON_ERROR);
        }
        if (c!='0') while (p->position<p->length && digit((unsigned char)p->data[p->position])) ++p->position;
        else if (p->position<p->length && digit((unsigned char)p->data[p->position])) return fail(p,TK_JSON_ERROR);
        if (p->position<p->length && p->data[p->position]=='.') {
            ++p->position;
            if (p->position>=p->length) return fail(p,TK_JSON_INCOMPLETE);
            if (!digit((unsigned char)p->data[p->position])) return fail(p,TK_JSON_ERROR);
            while (p->position<p->length && digit((unsigned char)p->data[p->position])) ++p->position;
        }
        if (p->position<p->length && (p->data[p->position]=='e'||p->data[p->position]=='E')) {
            ++p->position;
            if (p->position<p->length && (p->data[p->position]=='+'||p->data[p->position]=='-')) ++p->position;
            if (p->position>=p->length) return fail(p,TK_JSON_INCOMPLETE);
            if (!digit((unsigned char)p->data[p->position])) return fail(p,TK_JSON_ERROR);
            while (p->position<p->length && digit((unsigned char)p->data[p->position])) ++p->position;
        }
        t->type=TK_JSON_TOKEN_NUMBER; t->start=p->data+start; t->length=p->position-start;
        return TK_JSON_OK;
    } else {
        --p->position;
        if (c=='t') return word(p,t,"true",TK_JSON_TOKEN_TRUE);
        if (c=='f') return word(p,t,"false",TK_JSON_TOKEN_FALSE);
        if (c=='n') return word(p,t,"null",TK_JSON_TOKEN_NULL);
        return fail(p,TK_JSON_ERROR);
    }
    t->start=p->data+start; t->length=1; return TK_JSON_OK;
}
static int put(char *out,long size,long *used,int value)
{
    if (*used>=size-1) return TK_JSON_STRING_TOO_LONG;
    out[(*used)++]=(char)value; return TK_JSON_OK;
}
int TK_JsonDecodeString(const TKJsonToken *t,char *out,long size)
{
    long pos=0,used=0;
    if (!t || t->type!=TK_JSON_TOKEN_STRING || !out || size<=0) return TK_JSON_ERROR;
    while (pos<t->length) {
        unsigned int c=(unsigned char)t->start[pos++];
        if (c=='\\') {
            int a,b,d,e; unsigned int value;
            if (pos>=t->length) return TK_JSON_INCOMPLETE;
            c=(unsigned char)t->start[pos++];
            if (c=='b') c='\b'; else if (c=='f') c='\f'; else if (c=='n') c='\n';
            else if (c=='r') c='\r'; else if (c=='t') c='\t';
            else if (c=='u') {
                if (pos+4>t->length) return TK_JSON_INCOMPLETE;
                a=hex((unsigned char)t->start[pos]); b=hex((unsigned char)t->start[pos+1]);
                d=hex((unsigned char)t->start[pos+2]); e=hex((unsigned char)t->start[pos+3]);
                if (a<0||b<0||d<0||e<0) return TK_JSON_ERROR;
                pos+=4; value=(unsigned int)((a<<12)|(b<<8)|(d<<4)|e); c=value<=255?value:'?';
            } else if (c!='"' && c!='\\' && c!='/') return TK_JSON_ERROR;
        } else if (c>=0xC2 && c<=0xC3) {
            unsigned int n;
            if (pos>=t->length) return TK_JSON_INCOMPLETE;
            n=(unsigned char)t->start[pos++]; if (n<0x80||n>0xBF) return TK_JSON_ERROR;
            c=((c&0x1F)<<6)|(n&0x3F);
        } else if (c>=0x80) {
            int count=c>=0xE0&&c<=0xEF?2:(c>=0xF0&&c<=0xF4?3:-1); int i;
            if (count<0 || pos+count>t->length) return TK_JSON_ERROR;
            for (i=0;i<count;++i) { unsigned int n=(unsigned char)t->start[pos++]; if (n<0x80||n>0xBF) return TK_JSON_ERROR; }
            c='?';
        }
        if (put(out,size,&used,c)!=TK_JSON_OK) { out[used]=0; return TK_JSON_STRING_TOO_LONG; }
    }
    out[used]=0; return TK_JSON_OK;
}
int TK_JsonTokenEquals(const TKJsonToken *t,const char *text)
{
    char value[TK_JSON_MAX_STRING+1];
    if (!text || TK_JsonDecodeString(t,value,sizeof(value))<0) return 0;
    return strcmp(value,text)==0;
}
int TK_JsonSkipValue(TKJsonParser *p)
{
    TKJsonToken t; int r,count=0;
    r=TK_JsonNext(p,&t); if (r!=TK_JSON_OK) return r;
    if (t.type!=TK_JSON_TOKEN_OBJECT_BEGIN && t.type!=TK_JSON_TOKEN_ARRAY_BEGIN) return TK_JSON_OK;
    count=1;
    while (count) {
        r=TK_JsonNext(p,&t);
        if (r==TK_JSON_END) return fail(p,TK_JSON_INCOMPLETE);
        if (r!=TK_JSON_OK) return r;
        if (t.type==TK_JSON_TOKEN_OBJECT_BEGIN||t.type==TK_JSON_TOKEN_ARRAY_BEGIN) ++count;
        else if (t.type==TK_JSON_TOKEN_OBJECT_END||t.type==TK_JSON_TOKEN_ARRAY_END) --count;
    }
    return TK_JSON_OK;
}
static int value(TKJsonParser *p);
static int container(TKJsonParser *p,int object)
{
    TKJsonToken t; int r,first=1;
    for (;;) {
        long saved=p->position; int saved_depth=p->depth;
        r=TK_JsonNext(p,&t); if (r==TK_JSON_END) return fail(p,TK_JSON_INCOMPLETE);
        if (r!=TK_JSON_OK) return r;
        if ((object&&t.type==TK_JSON_TOKEN_OBJECT_END)||(!object&&t.type==TK_JSON_TOKEN_ARRAY_END)) return TK_JSON_OK;
        p->position=saved; p->depth=saved_depth;
        if (!first) { r=TK_JsonNext(p,&t); if (r!=TK_JSON_OK||t.type!=TK_JSON_TOKEN_COMMA) return fail(p,TK_JSON_ERROR); }
        if (object) {
            r=TK_JsonNext(p,&t); if (r!=TK_JSON_OK||t.type!=TK_JSON_TOKEN_STRING) return fail(p,r==TK_JSON_END?TK_JSON_INCOMPLETE:TK_JSON_ERROR);
            r=TK_JsonNext(p,&t); if (r!=TK_JSON_OK||t.type!=TK_JSON_TOKEN_COLON) return fail(p,r==TK_JSON_END?TK_JSON_INCOMPLETE:TK_JSON_ERROR);
        }
        r=value(p); if (r!=TK_JSON_OK) return r; first=0;
    }
}
static int value(TKJsonParser *p)
{
    TKJsonToken t; int r=TK_JsonNext(p,&t);
    if (r==TK_JSON_END) return fail(p,TK_JSON_INCOMPLETE);
    if (r!=TK_JSON_OK) return r;
    if (t.type==TK_JSON_TOKEN_OBJECT_BEGIN) return container(p,1);
    if (t.type==TK_JSON_TOKEN_ARRAY_BEGIN) return container(p,0);
    if (t.type==TK_JSON_TOKEN_STRING||t.type==TK_JSON_TOKEN_NUMBER||t.type==TK_JSON_TOKEN_TRUE||t.type==TK_JSON_TOKEN_FALSE||t.type==TK_JSON_TOKEN_NULL) return TK_JSON_OK;
    return fail(p,TK_JSON_ERROR);
}
int TK_JsonValidate(TKJsonParser *p)
{
    TKJsonToken t; int r;
    if (!p||p->error<0) return p?p->error:TK_JSON_ERROR;
    r=value(p); if (r!=TK_JSON_OK) return r;
    r=TK_JsonNext(p,&t); if (r!=TK_JSON_END) return fail(p,TK_JSON_ERROR);
    return p->depth==0?TK_JSON_OK:fail(p,TK_JSON_INCOMPLETE);
}
const char *TK_JsonErrorText(int e)
{
    if (e==TK_JSON_OK) return "JSON valid";
    if (e==TK_JSON_INCOMPLETE) return "Incomplete JSON response";
    if (e==TK_JSON_DEPTH_EXCEEDED) return "JSON nesting too deep";
    if (e==TK_JSON_STRING_TOO_LONG) return "JSON string too long";
    return "Malformed JSON response";
}
