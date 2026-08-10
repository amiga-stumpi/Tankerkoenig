#ifndef TANKERKOENIG_GEOCODE_H
#define TANKERKOENIG_GEOCODE_H
#include <exec/types.h>
#include "https.h"
#define TK_GEOCODE_MAX_RESULTS 4
#define TK_GEOCODE_TEXT_SIZE 64
#define TK_GEOCODE_COORD_SIZE 20
#define TK_GEOCODE_OK 0
#define TK_GEOCODE_NO_RESULTS 1
#define TK_GEOCODE_BAD_QUERY -20
#define TK_GEOCODE_INVALID_JSON -21
#define TK_GEOCODE_BAD_RESPONSE -22
typedef struct TKLocationResult {
    char name[TK_GEOCODE_TEXT_SIZE];
    char admin[TK_GEOCODE_TEXT_SIZE];
    char country[TK_GEOCODE_TEXT_SIZE];
    char country_code[3];
    char latitude[TK_GEOCODE_COORD_SIZE];
    char longitude[TK_GEOCODE_COORD_SIZE];
} TKLocationResult;
typedef struct TKLocationResults {
    TKLocationResult items[TK_GEOCODE_MAX_RESULTS];
    WORD count;
} TKLocationResults;
int TK_GeocodeSearch(TKHttpsClient *, const char *, UBYTE *, LONG,
    TKLocationResults *);
int TK_GeocodeDecode(const char *, LONG, TKLocationResults *);
const char *TK_GeocodeErrorText(int);
#endif
