#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"
static char *load(const char *path,long *length)
{
    FILE *f; char *data; long size;
    f=fopen(path,"rb"); if (!f) return 0;
    if (fseek(f,0,SEEK_END)||((size=ftell(f))<0)||fseek(f,0,SEEK_SET)) { fclose(f); return 0; }
    data=(char *)malloc((size_t)size+1); if (!data) { fclose(f); return 0; }
    if ((long)fread(data,1,(size_t)size,f)!=size) { free(data); fclose(f); return 0; }
    fclose(f); data[size]=0; *length=size; return data;
}
static int fixture(const char *dir,const char *name,int expected)
{
    char path[512],*data; long length; TKJsonParser p; int result;
    sprintf(path,"%s/%s",dir,name); data=load(path,&length);
    if (!data) { printf("FAIL cannot read %s\n",path); return 0; }
    TK_JsonInit(&p,data,length); result=TK_JsonValidate(&p); free(data);
    if (result!=expected) { printf("FAIL %s expected %d got %d\n",name,expected,result); return 0; }
    return 1;
}
static int decode(void)
{
    const char raw[]="Osnabr\\u00fcck, K\\u00f6ln, M\303\274nchen";
    const unsigned char expected[]={'O','s','n','a','b','r',0xfc,'c','k',',',' ','K',0xf6,'l','n',',',' ','M',0xfc,'n','c','h','e','n',0};
    TKJsonToken t; char out[64];
    t.type=TK_JSON_TOKEN_STRING; t.start=raw; t.length=(long)strlen(raw);
    if (TK_JsonDecodeString(&t,out,sizeof(out))!=TK_JSON_OK || memcmp(out,expected,sizeof(expected))) { printf("FAIL Latin-1 decoding\n"); return 0; }
    return 1;
}
static int limits(void)
{
    char deep[80], long_string[TK_JSON_MAX_STRING + 4]; int i; TKJsonParser p;
    for (i=0;i<TK_JSON_MAX_DEPTH+1;++i) deep[i]='[';
    for (i=0;i<TK_JSON_MAX_DEPTH+1;++i) deep[TK_JSON_MAX_DEPTH+1+i]=']';
    deep[(TK_JSON_MAX_DEPTH+1)*2]=0;
    TK_JsonInit(&p,deep,(long)strlen(deep));
    if (TK_JsonValidate(&p)!=TK_JSON_DEPTH_EXCEEDED) { puts("FAIL depth limit"); return 0; }
    long_string[0]='"';
    for (i=0;i<TK_JSON_MAX_STRING+1;++i) long_string[i+1]='a';
    long_string[TK_JSON_MAX_STRING+2]='"'; long_string[TK_JSON_MAX_STRING+3]=0;
    TK_JsonInit(&p,long_string,(long)strlen(long_string));
    if (TK_JsonValidate(&p)!=TK_JSON_STRING_TOO_LONG) { puts("FAIL string limit"); return 0; }
    return 1;
}
static int skip(void)
{
    const char data[]="{\"unknown\":{\"nested\":[1,true,null]},\"name\":\"Berlin\"}";
    TKJsonParser p; TKJsonToken t;
    TK_JsonInit(&p,data,(long)strlen(data));
    if (TK_JsonNext(&p,&t)!=TK_JSON_OK||t.type!=TK_JSON_TOKEN_OBJECT_BEGIN) return 0;
    if (TK_JsonNext(&p,&t)!=TK_JSON_OK||!TK_JsonTokenEquals(&t,"unknown")) return 0;
    if (TK_JsonNext(&p,&t)!=TK_JSON_OK||t.type!=TK_JSON_TOKEN_COLON) return 0;
    if (TK_JsonSkipValue(&p)!=TK_JSON_OK) return 0;
    if (TK_JsonNext(&p,&t)!=TK_JSON_OK||t.type!=TK_JSON_TOKEN_COMMA) return 0;
    return 1;
}
int main(int argc,char **argv)
{
    int ok=1;
    if (argc!=2) return 2;
    ok&=fixture(argv[1],"valid.json",TK_JSON_OK);
    ok&=fixture(argv[1],"incomplete.json",TK_JSON_INCOMPLETE);
    ok&=fixture(argv[1],"malformed.json",TK_JSON_ERROR);
    ok&=decode(); ok&=skip(); ok&=limits();
    if (!ok) return 1;
    puts("JSON parser tests passed"); return 0;
}
