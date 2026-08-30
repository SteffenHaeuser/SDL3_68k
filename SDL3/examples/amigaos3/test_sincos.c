#include <SDL3/SDL.h>
#include <stdio.h>

int main(void)
{
    const double pi = SDL_PI_D;
    const double angles[] = { 0.0, pi * 0.5, pi, pi * 1.5, pi * 2.0, -pi * 0.5 };
    int i;

    for (i = 0; i < (int)SDL_arraysize(angles); ++i) {
        printf("x=% .9f sin=% .9f cos=% .9f\n",
               angles[i], SDL_sin(angles[i]), SDL_cos(angles[i]));
    }
    return 0;
}
