#include <stdio.h>
#include <string.h>
#include "geocode.h"
#include "json.h"
static const char response[]=
"{\"generationtime_ms\":0.1,\"results\":["
"{\"id\":1,\"name\":\"Osnabr\\u00fcck\",\"latitude\":52.2799,\"longitude\":8.0472,\"admin1\":\"Niedersachsen\",\"country\":\"Deutschland\",\"unknown\":{\"a\":[1,2]}},"
"{\"name\":\"Köln\",\"latitude\":50.94,\"longitude\":6.96,\"country\":\"Deutschland\"},"
"{\"name\":\"Berlin\",\"latitude\":52.52,\"longitude\":13.405},"
"{\"name\":\"Hamburg\",\"latitude\":53.55,\"longitude\":9.99},"
"{\"name\":\"München\",\"latitude\":48.14,\"longitude\":11.58}]}";
static char requested_url[768];
int TK_HttpsGet(TKHttpsClient *client,const char *url,UBYTE *output,LONG size,LONG *length)
{
    long n=(long)strlen(response); (void)client;
    if (n>=size) return TK_HTTPS_RESPONSE_TOO_LARGE;
    strcpy(requested_url,url); memcpy(output,response,(size_t)n+1); *length=n; return TK_HTTPS_OK;
}
const char *TK_HttpsErrorText(int error) { (void)error; return "HTTPS error"; }
int main(void)
{
    TKHttpsClient https; TKLocationResults results; UBYTE buffer[4096]; int r; TKJsonParser parser;
    TK_JsonInit(&parser,response,(long)strlen(response)); r=TK_JsonValidate(&parser); if (r!=TK_JSON_OK) { printf("FAIL source JSON %d pos %ld\n",r,parser.position); return 1; }
    memset(&https,0,sizeof(https)); https.last_http_status=200;
    r=TK_GeocodeSearch(&https,"Osnabr\374ck",buffer,sizeof(buffer),&results);
    if (r!=TK_GEOCODE_OK) { printf("FAIL result %d\n",r); return 1; }
    if (!strstr(requested_url,"name=Osnabr%C3%BCck")||!strstr(requested_url,"count=4")) { printf("FAIL URL %s\n",requested_url); return 1; }
    r=TK_GeocodeSearch(&https,"M\303\274nster",buffer,sizeof(buffer),&results);
    if (r!=TK_GEOCODE_OK||!strstr(requested_url,"name=M%C3%BCnster")) { printf("FAIL UTF-8 URL %s\n",requested_url); return 1; }
    if (results.count!=4) { printf("FAIL count %d\n",results.count); return 1; }
    if ((unsigned char)results.items[0].name[6]!=0xfc || strcmp(results.items[0].latitude,"52.2799")) { puts("FAIL first result"); return 1; }
    if ((unsigned char)results.items[1].name[1]!=0xf6) { puts("FAIL UTF-8 result"); return 1; }
    r=TK_GeocodeDecode("{\"results\":[]}",(LONG)strlen("{\"results\":[]}"),&results);
    if (r!=TK_GEOCODE_NO_RESULTS) { printf("FAIL empty result %d\n",r); return 1; }
    puts("Geocode tests passed"); return 0;
}
