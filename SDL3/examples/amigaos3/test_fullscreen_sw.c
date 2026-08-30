#include <SDL3/SDL.h>
#include <stdio.h>

int main(void)
{
    SDL_Window *w;
    SDL_Renderer *r;
    SDL_Event e;
    bool run = true;
    bool fullscreen = false;
    Uint32 frame = 0;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    w = SDL_CreateWindow("SDL3 AmigaOS3 Fullscreen SW", 640, 480, SDL_WINDOW_RESIZABLE);
    if (!w) {
        printf("window failed: %s\n", SDL_GetError());
        return 2;
    }

    r = SDL_CreateRenderer(w, "software");
    if (!r) {
        printf("renderer failed: %s\n", SDL_GetError());
        return 3;
    }

    printf("F toggles fullscreen, ESC exits\n");

    while (run) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) run = false;
            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_ESCAPE) run = false;
                if (e.key.key == SDLK_F) {
                    fullscreen = !fullscreen;
                    if (!SDL_SetWindowFullscreen(w, fullscreen)) {
                        printf("fullscreen transition failed: %s\n", SDL_GetError());
                        fullscreen = !fullscreen;
                    } else {
                        printf("fullscreen %s\n", fullscreen ? "ON" : "OFF");
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(r, (Uint8)(frame & 255), 30, 90, 255);
        SDL_RenderClear(r);
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        {
            SDL_FRect box = { 50.0f, 50.0f, 200.0f, 120.0f };
            SDL_RenderFillRect(r, &box);
        }
        SDL_RenderPresent(r);
        ++frame;
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);
    SDL_Quit();
    return 0;
}
