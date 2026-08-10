#ifndef TANKERKOENIG_CONFIG_H
#define TANKERKOENIG_CONFIG_H
#include <exec/types.h>
#define TK_CONFIG_FILE "Tankerkoenig.conf"
#define TK_API_KEY_SIZE 40
#define TK_LOCATION_SIZE 64
#define TK_COORD_SIZE 20
#define TK_FUEL_E5 0
#define TK_FUEL_E10 1
#define TK_FUEL_DIESEL 2
#define TK_FUEL_ALL 3
#define TK_SORT_PRICE 0
#define TK_SORT_DISTANCE 1
typedef struct TKConfig {
    char api_key[TK_API_KEY_SIZE];
    char location[TK_LOCATION_SIZE];
    char latitude[TK_COORD_SIZE];
    char longitude[TK_COORD_SIZE];
    WORD radius;
    WORD fuel;
    WORD sort;
    WORD open_only;
    WORD update_minutes;
} TKConfig;
void TK_ConfigDefaults(TKConfig *config);
int TK_LoadConfig(TKConfig *config);
int TK_SaveConfig(const TKConfig *config);
int TK_ConfigHasApiKey(const TKConfig *config);
const char *TK_ConfigFuelName(WORD fuel);
const char *TK_ConfigSortName(WORD sort);
#endif
