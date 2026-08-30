
#include <proto/dos.h>
#include <stdio.h>
int main(void)
{
    PutStr("ctor_stdio: entered main()\n");
    fprintf(stderr, "ctor_stdio: fprintf works\n");
    PutStr("ctor_stdio: returning\n");
    return 0;
}
