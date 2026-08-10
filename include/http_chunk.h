#ifndef TANKERKOENIG_HTTP_CHUNK_H
#define TANKERKOENIG_HTTP_CHUNK_H
#define TK_HTTP_CHUNK_MALFORMED -1
#define TK_HTTP_CHUNK_INCOMPLETE 0
#define TK_HTTP_CHUNK_COMPLETE 1
int TK_HttpChunkComplete(const char *,long,long *);
int TK_HttpChunkDecode(char *,long,long *);
#endif
