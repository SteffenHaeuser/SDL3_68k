#include <SDL3/SDL.h>
#include <stdio.h>

static void dump_modes(void)
{
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    int count = 0, i;
    SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(display, &count);

    printf("primary display=%lu fullscreen modes=%d\n",
           (unsigned long)display, count);

    for (i = 0; i < count; ++i) {
        printf("%2d: %dx%d format=%s refresh=%.1f\n",
               i, modes[i]->w, modes[i]->h,
               SDL_GetPixelFormatName(modes[i]->format),
               modes[i]->refresh_rate);
    }
    SDL_free(modes);
}

int main(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    dump_modes();
    SDL_Quit();
    return 0;
}
