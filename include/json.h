#ifndef TANKERKOENIG_JSON_H
#define TANKERKOENIG_JSON_H

#define TK_JSON_MAX_DEPTH 16
#define TK_JSON_MAX_STRING 255
#define TK_JSON_MAX_RESULTS 64
#define TK_JSON_OK 0
#define TK_JSON_END 1
#define TK_JSON_ERROR -1
#define TK_JSON_INCOMPLETE -2
#define TK_JSON_DEPTH_EXCEEDED -3
#define TK_JSON_STRING_TOO_LONG -4

typedef enum TKJsonTokenType {
    TK_JSON_TOKEN_NONE = 0, TK_JSON_TOKEN_OBJECT_BEGIN,
    TK_JSON_TOKEN_OBJECT_END, TK_JSON_TOKEN_ARRAY_BEGIN,
    TK_JSON_TOKEN_ARRAY_END, TK_JSON_TOKEN_STRING, TK_JSON_TOKEN_NUMBER,
    TK_JSON_TOKEN_TRUE, TK_JSON_TOKEN_FALSE, TK_JSON_TOKEN_NULL,
    TK_JSON_TOKEN_COLON, TK_JSON_TOKEN_COMMA
} TKJsonTokenType;

typedef struct TKJsonToken {
    TKJsonTokenType type;
    const char *start;
    long length;
} TKJsonToken;

typedef struct TKJsonParser {
    const char *data;
    long length;
    long position;
    int depth;
    int error;
} TKJsonParser;

void TK_JsonInit(TKJsonParser *, const char *, long);
int TK_JsonNext(TKJsonParser *, TKJsonToken *);
int TK_JsonValidate(TKJsonParser *);
int TK_JsonSkipValue(TKJsonParser *);
int TK_JsonDecodeString(const TKJsonToken *, char *, long);
int TK_JsonTokenEquals(const TKJsonToken *, const char *);
const char *TK_JsonErrorText(int);
#endif
