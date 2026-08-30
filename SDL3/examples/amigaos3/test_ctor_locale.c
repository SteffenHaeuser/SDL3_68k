
#include <proto/dos.h>
#include <ctype.h>
volatile int c = 'a';
int main(void)
{
    int r;
    PutStr("ctor_locale: entered main()\n");
    r = toupper(c);
    (void)r;
    PutStr("ctor_locale: ctype call returned\n");
    return 0;
}
