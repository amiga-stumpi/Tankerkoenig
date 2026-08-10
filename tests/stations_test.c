#include <stdio.h>
#include <string.h>
#include "stations.h"
static const char *mock_response;
static char requested_url[1200];
int TK_HttpsGet(TKHttpsClient *client,const char *url,UBYTE *output,LONG size,LONG *length)
{
    long n=(long)strlen(mock_response); (void)client;
    if (n>=size) return TK_HTTPS_RESPONSE_TOO_LARGE;
    strcpy(requested_url,url); memcpy(output,mock_response,(size_t)n+1); *length=n; return TK_HTTPS_OK;
}
const char *TK_HttpsErrorText(int error) { (void)error; return "HTTPS error"; }
int TK_ConfigHasApiKey(const TKConfig *config) { return config&&config->api_key[0]; }
const char *TK_ConfigFuelName(WORD fuel)
{
    if (fuel==TK_FUEL_E5) return "e5";
    if (fuel==TK_FUEL_DIESEL) return "diesel";
    if (fuel==TK_FUEL_ALL) return "all";
    return "e10";
}
const char *TK_ConfigSortName(WORD sort) { return sort==TK_SORT_DISTANCE?"dist":"price"; }
static int decode_tests(TKConfig *config)
{
    static const char response_a[]=
    "{\"ok\":true,\"license\":\"CC BY 4.0\",\"stations\":["
    "{\"id\":\"one\",\"name\":\"Station Ä\",\"brand\":null,\"street\":\"Main\",\"houseNumber\":\"2\",\"postCode\":48143,\"place\":\"Münster\",\"dist\":1.2,\"price\":1.679,\"isOpen\":true,\"extra\":{\"x\":1}},"
    "{\"id\":\"two\",\"name\":\"Closed\",\"dist\":2,\"price\":false,\"isOpen\":false}]}";
    static const char response_all[]=
    "{\"stations\":[{\"name\":\"All Fuels\",\"diesel\":1.599,\"e5\":1.799,\"e10\":false,\"isOpen\":true}],\"ok\":true}";
    TKStationResults results; int r;
    r=TK_StationsDecode(response_a,(LONG)strlen(response_a),config,&results);
    if (r!=TK_STATIONS_OK||results.count!=2||strcmp(results.items[0].price,"1.679")||results.items[1].price[0]) { puts("FAIL station decode"); return 0; }
    config->open_only=1; r=TK_StationsDecode(response_a,(LONG)strlen(response_a),config,&results); config->open_only=0;
    if (r!=TK_STATIONS_OK||results.count!=1) { puts("FAIL open filter"); return 0; }
    r=TK_StationsDecode(response_all,(LONG)strlen(response_all),config,&results);
    if (r!=TK_STATIONS_OK||strcmp(results.items[0].diesel,"1.599")||results.items[0].e10[0]) { puts("FAIL all fuels"); return 0; }
    r=TK_StationsDecode("{\"ok\":false,\"message\":\"bad key\"}",(LONG)strlen("{\"ok\":false,\"message\":\"bad key\"}"),config,&results);
    if (r!=TK_STATIONS_API_REJECTED) { puts("FAIL API rejection"); return 0; }
    return 1;
}
int main(void)
{
    static const char response[]="{\"ok\":true,\"stations\":[{\"name\":\"Test\",\"price\":1.7,\"isOpen\":true}]}";
    TKConfig config; TKStationResults results; TKHttpsClient https; UBYTE buffer[2048]; int r;
    memset(&config,0,sizeof(config)); memset(&https,0,sizeof(https));
    strcpy(config.api_key,"11111111-1111-1111-1111-111111111111"); strcpy(config.latitude,"51.96"); strcpy(config.longitude,"7.62");
    config.radius=10; config.fuel=TK_FUEL_E10; config.sort=TK_SORT_PRICE;
    if (!decode_tests(&config)) return 1;
    mock_response=response; https.last_http_status=200;
    r=TK_StationsSearch(&https,&config,buffer,sizeof(buffer),&results);
    if (r!=TK_STATIONS_OK||!strstr(requested_url,"lat=51.96&lng=7.62&rad=10&type=e10&sort=price&apikey=")) { printf("FAIL request %s\n",requested_url); return 1; }
    config.fuel=TK_FUEL_ALL; r=TK_StationsSearch(&https,&config,buffer,sizeof(buffer),&results);
    if (r!=TK_STATIONS_OK||strstr(requested_url,"sort=")||!strstr(requested_url,"type=all")) { puts("FAIL all request"); return 1; }
    puts("Station API tests passed"); return 0;
}
