#include <SDL3/SDL.h>
#include <stdio.h>
#include <proto/dos.h>

int main(int argc, char **argv)
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
    bool running = true;
    float x = 20.0f;
    float dx = 2.0f;

    (void)argc;
    (void)argv;

    PutStr("test_sw: entered main()\n");
    PutStr("test_sw: before SDL_Init()\n");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        PutStr("test_sw: SDL_Init FAILED\n");
        printf("SDL error: %s\n", SDL_GetError());
        fflush(stdout);
        return 1;
    }
    PutStr("test_sw: SDL_Init returned OK\n");
    printf("test_sw: video driver = %s\n", SDL_GetCurrentVideoDriver());
    fflush(stdout);

    PutStr("test_sw: before SDL_CreateWindow()\n");
    window = SDL_CreateWindow("SDL3 AmigaOS3 Software Test", 320, 240, 0);
    if (!window) {
        PutStr("test_sw: SDL_CreateWindow FAILED\n");
        printf("SDL error: %s\n", SDL_GetError());
        fflush(stdout);
        SDL_Quit();
        return 1;
    }
    PutStr("test_sw: window created\n");

    PutStr("test_sw: before SDL_CreateRenderer(software)\n");
    renderer = SDL_CreateRenderer(window, "software");
    if (!renderer) {
        PutStr("test_sw: SDL_CreateRenderer FAILED\n");
        printf("SDL error: %s\n", SDL_GetError());
        fflush(stdout);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    PutStr("test_sw: renderer created, entering loop\n");

    while (running) {
        SDL_FRect r;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 24, 24, 40, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 0, 160, 255, 255);
        r.x = x;
        r.y = 40.0f;
        r.w = 80.0f;
        r.h = 50.0f;
        SDL_RenderFillRect(renderer, &r);

        SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255);
        r.x = 320.0f - x - 60.0f;
        r.y = 140.0f;
        r.w = 60.0f;
        r.h = 40.0f;
        SDL_RenderFillRect(renderer, &r);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderLine(renderer, 0.0f, 0.0f, 319.0f, 239.0f);
        SDL_RenderLine(renderer, 0.0f, 239.0f, 319.0f, 0.0f);

        SDL_RenderPresent(renderer);

        x += dx;
        if (x <= 0.0f || x >= 220.0f) {
            dx = -dx;
        }
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
