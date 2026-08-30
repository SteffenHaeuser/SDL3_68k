#include <SDL3/SDL.h>
#include <stdio.h>

static double dabs(double x)
{
    return x < 0.0 ? -x : x;
}

static int report(const char *name, double got, double expected, double tolerance)
{
    double err = dabs(got - expected);
    int ok = (err <= tolerance);

    /*
     * Do not print doubles here. On this m68k/clib2 target the floating-point
     * printf conversion itself is under suspicion and would contaminate the
     * math test.
     */
    printf("%-8s %s\n", name, ok ? "OK" : "FAIL");
    fflush(stdout);
    return ok ? 0 : 1;
}

#define RUN_TEST(label, expression, expected, tolerance) \
    do { \
        double value; \
        printf("TEST %-8s ...\n", label); \
        fflush(stdout); \
        value = (expression); \
        fails += report(label, value, expected, tolerance); \
    } while (0)

int main(void)
{
    int fails = 0;
    const double pi = SDL_PI_D;

    puts("SDL3 AmigaOS3 math wrapper test (no float printf)");
    fflush(stdout);

    RUN_TEST("sin",    SDL_sin(pi / 6.0),             0.5,                0.00002);
    RUN_TEST("cos",    SDL_cos(pi / 3.0),             0.5,                0.00002);
    RUN_TEST("tan",    SDL_tan(pi / 4.0),             1.0,                0.000001);
    RUN_TEST("atan",   SDL_atan(1.0),                 pi / 4.0,           0.000001);
    RUN_TEST("atan2",  SDL_atan2(1.0, 1.0),           pi / 4.0,           0.000001);
    RUN_TEST("ceil",   SDL_ceil(1.25),                2.0,                0.0);
    RUN_TEST("floor",  SDL_floor(1.75),               1.0,                0.0);
    RUN_TEST("fmod",   SDL_fmod(7.0, 2.0),            1.0,                0.000001);
    RUN_TEST("exp",    SDL_exp(1.0),                  2.718281828459045,  0.000001);
    RUN_TEST("log",    SDL_log(2.718281828459045),    1.0,                0.000001);
    RUN_TEST("log10",  SDL_log10(1000.0),             3.0,                0.000001);
    RUN_TEST("pow",    SDL_pow(2.0, 10.0),            1024.0,             0.000001);
    RUN_TEST("sqrt",   SDL_sqrt(2.0),                 1.4142135623730951, 0.000001);

    puts("Extra sign/quadrant tests");
    fflush(stdout);

    RUN_TEST("sin-",   SDL_sin(-pi / 2.0),           -1.0,               0.00002);
    RUN_TEST("cospi",  SDL_cos(pi),                   -1.0,               0.00002);
    RUN_TEST("atan2Q", SDL_atan2(1.0, -1.0),          3.0 * pi / 4.0,     0.000001);
    RUN_TEST("floor-", SDL_floor(-1.25),              -2.0,               0.0);
    RUN_TEST("ceil-",  SDL_ceil(-1.25),               -1.0,               0.0);

    printf("RESULT: %s (%d failures)\n", fails ? "FAIL" : "OK", fails);
    return fails ? 1 : 0;
}
