#ifndef TANKERKOENIG_STATIONS_H
#define TANKERKOENIG_STATIONS_H
#include <exec/types.h>
#include "config.h"
#include "https.h"
#define TK_STATION_MAX_RESULTS 16
#define TK_STATION_TEXT_SIZE 96
#define TK_STATION_PRICE_SIZE 16
#define TK_STATIONS_OK 0
#define TK_STATIONS_NO_RESULTS 1
#define TK_STATIONS_MISSING_KEY -30
#define TK_STATIONS_INVALID_JSON -31
#define TK_STATIONS_BAD_RESPONSE -32
#define TK_STATIONS_API_REJECTED -33
typedef struct TKStation {
    char id[40];
    char name[TK_STATION_TEXT_SIZE];
    char brand[TK_STATION_TEXT_SIZE];
    char street[TK_STATION_TEXT_SIZE];
    char house_number[16];
    char post_code[16];
    char place[TK_STATION_TEXT_SIZE];
    char distance[TK_STATION_PRICE_SIZE];
    char price[TK_STATION_PRICE_SIZE];
    char e5[TK_STATION_PRICE_SIZE];
    char e10[TK_STATION_PRICE_SIZE];
    char diesel[TK_STATION_PRICE_SIZE];
    UBYTE is_open;
} TKStation;
typedef struct TKStationResults {
    TKStation items[TK_STATION_MAX_RESULTS];
    WORD count;
} TKStationResults;
int TK_StationsSearch(TKHttpsClient *,const TKConfig *,UBYTE *,LONG,TKStationResults *);
int TK_StationsDecode(const char *,LONG,const TKConfig *,TKStationResults *);
const char *TK_StationsErrorText(int);
#endif
