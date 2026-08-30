#include <SDL3/SDL.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    SDL_Window *window;
    SDL_Surface *surface;
    SDL_Event event;
    bool running = true;
    Uint32 frame = 0;

    (void)argc;
    (void)argv;

    printf("SDL_Init...\n");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    printf("video driver: %s\n", SDL_GetCurrentVideoDriver());

    window = SDL_CreateWindow("SDL3 AmigaOS3 Surface Test", 640, 480, 0);
    if (!window) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    printf("window created\n");

    surface = SDL_GetWindowSurface(window);
    if (!surface) {
        printf("SDL_GetWindowSurface failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    printf("surface: %dx%d pitch=%d\n", surface->w, surface->h, surface->pitch);

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            }
        }

        SDL_FillSurfaceRect(surface, NULL,
            SDL_MapSurfaceRGB(surface,
                (Uint8)((frame >> 1) & 255),
                (Uint8)((frame >> 2) & 255),
                (Uint8)((frame >> 3) & 255)));

        SDL_UpdateWindowSurface(window);
        ++frame;
        SDL_Delay(16);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
