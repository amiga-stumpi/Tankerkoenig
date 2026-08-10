#include <stdio.h>
#include <string.h>
#include "http_chunk.h"
static int check(const char *source,const char *expected)
{
    char data[256]; long encoded=0,decoded=0; int result;
    strcpy(data,source); result=TK_HttpChunkComplete(data,(long)strlen(data),&encoded);
    if (result!=TK_HTTP_CHUNK_COMPLETE||encoded!=(long)strlen(data)) return 0;
    result=TK_HttpChunkDecode(data,encoded,&decoded);
    return result==TK_HTTP_CHUNK_COMPLETE&&decoded==(long)strlen(expected)&&!memcmp(data,expected,(size_t)decoded);
}
int main(void)
{
    long length;
    if (!check("4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n","Wikipedia")) { puts("FAIL basic chunks"); return 1; }
    if (!check("3;x=y\r\nabc\r\n0\r\nHeader: value\r\n\r\n","abc")) { puts("FAIL extension/trailer"); return 1; }
    if (TK_HttpChunkComplete("4\r\nWi",5,&length)!=TK_HTTP_CHUNK_INCOMPLETE) { puts("FAIL incomplete"); return 1; }
    if (TK_HttpChunkComplete("Z\r\n",3,&length)!=TK_HTTP_CHUNK_MALFORMED) { puts("FAIL malformed"); return 1; }
    puts("HTTP chunk tests passed"); return 0;
}
