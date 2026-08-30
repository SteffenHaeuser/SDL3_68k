
#include <proto/dos.h>
#include <math.h>
volatile double x = 0.5;
int main(void)
{
    double y;
    PutStr("ctor_math: entered main()\n");
    y = sin(x) + cos(x);
    (void)y;
    PutStr("ctor_math: math call returned\n");
    return 0;
}
