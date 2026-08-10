#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
LONG FPuts(BPTR fh, CONST_STRPTR str);
LONG __stack = 8192;
int main(void)
{
    LONG ok = Execute((STRPTR)"stack 131072\ntkcore\n", 0, 0);
    if (!ok) {
        FPuts(Output(), (STRPTR)"Tankerkoenig: cannot execute tkcore\n");
        return 20;
    }
    return 0;
}
