
#include <proto/dos.h>
volatile unsigned int a = 1;
int main(void)
{
    unsigned int old;
    PutStr("ctor_atomic: entered main()\n");
    old = __atomic_exchange_n(&a, 2, __ATOMIC_SEQ_CST);
    (void)old;
    PutStr("ctor_atomic: atomic call returned\n");
    return 0;
}
