#include <string.h>
#include "http_chunk.h"
static int hex_value(int c)
{
    if (c>='0'&&c<='9') return c-'0';
    if (c>='a'&&c<='f') return c-'a'+10;
    if (c>='A'&&c<='F') return c-'A'+10;
    return -1;
}
static int chunk_size(const char *data,long length,long *position,unsigned long *size)
{
    int digits=0,value;
    *size=0;
    while (*position<length) {
        int c=(unsigned char)data[(*position)++];
        if (c==';') {
            while (*position<length&&data[*position]!='\n') ++*position;
            if (*position>=length) return TK_HTTP_CHUNK_INCOMPLETE;
            ++*position; return digits?TK_HTTP_CHUNK_COMPLETE:TK_HTTP_CHUNK_MALFORMED;
        }
        if (c=='\r') {
            if (*position>=length) return TK_HTTP_CHUNK_INCOMPLETE;
            if (data[(*position)++]!='\n') return TK_HTTP_CHUNK_MALFORMED;
            return digits?TK_HTTP_CHUNK_COMPLETE:TK_HTTP_CHUNK_MALFORMED;
        }
        if (c=='\n') return digits?TK_HTTP_CHUNK_COMPLETE:TK_HTTP_CHUNK_MALFORMED;
        value=hex_value(c); if (value<0) return TK_HTTP_CHUNK_MALFORMED;
        if (*size>0x0FFFFFFFUL) return TK_HTTP_CHUNK_MALFORMED;
        *size=(*size<<4)|(unsigned long)value; ++digits;
    }
    return TK_HTTP_CHUNK_INCOMPLETE;
}
int TK_HttpChunkComplete(const char *data,long length,long *encoded_length)
{
    long pos=0; unsigned long size; int result;
    if (!data||length<0||!encoded_length) return TK_HTTP_CHUNK_MALFORMED;
    for (;;) {
        result=chunk_size(data,length,&pos,&size); if (result!=TK_HTTP_CHUNK_COMPLETE) return result;
        if (!size) {
            for (;;) {
                long start=pos;
                while (pos<length&&data[pos]!='\n') ++pos;
                if (pos>=length) return TK_HTTP_CHUNK_INCOMPLETE;
                if (pos==start||(pos==start+1&&data[start]=='\r')) { ++pos; *encoded_length=pos; return TK_HTTP_CHUNK_COMPLETE; }
                ++pos;
            }
        }
        if (size>(unsigned long)(length-pos)) return TK_HTTP_CHUNK_INCOMPLETE;
        pos+=(long)size;
        if (pos>=length) return TK_HTTP_CHUNK_INCOMPLETE;
        if (data[pos]=='\r') {
            if (pos+1>=length) return TK_HTTP_CHUNK_INCOMPLETE;
            if (data[pos+1]!='\n') return TK_HTTP_CHUNK_MALFORMED;
            pos+=2;
        } else if (data[pos]=='\n') ++pos;
        else return TK_HTTP_CHUNK_MALFORMED;
    }
}
int TK_HttpChunkDecode(char *data,long length,long *decoded_length)
{
    long pos=0,out=0; unsigned long size; int result;
    if (!data||length<0||!decoded_length) return TK_HTTP_CHUNK_MALFORMED;
    for (;;) {
        result=chunk_size(data,length,&pos,&size); if (result!=TK_HTTP_CHUNK_COMPLETE) return result;
        if (!size) { *decoded_length=out; return TK_HTTP_CHUNK_COMPLETE; }
        if (size>(unsigned long)(length-pos)) return TK_HTTP_CHUNK_INCOMPLETE;
        memmove(data+out,data+pos,(long)size); out+=(long)size; pos+=(long)size;
        if (pos>=length) return TK_HTTP_CHUNK_INCOMPLETE;
        if (data[pos]=='\r') {
            if (pos+1>=length) return TK_HTTP_CHUNK_INCOMPLETE;
            if (data[pos+1]!='\n') return TK_HTTP_CHUNK_MALFORMED;
            pos+=2;
        } else if (data[pos]=='\n') ++pos;
        else return TK_HTTP_CHUNK_MALFORMED;
    }
}
