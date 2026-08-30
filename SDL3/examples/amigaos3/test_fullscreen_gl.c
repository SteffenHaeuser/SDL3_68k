#include <SDL3/SDL.h>
#include <proto/minigl.h>
#include <stdio.h>

int main(void)
{
    SDL_DisplayID display;
    SDL_DisplayMode **modes;
    const SDL_DisplayMode *chosen = NULL;
    int count = 0, i;
    SDL_Window *w;
    SDL_GLContext ctx;
    SDL_Event e;
    bool run = true;
    int ww = 640, wh = 480;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    display = SDL_GetPrimaryDisplay();
    modes = SDL_GetFullscreenDisplayModes(display, &count);

    /*
     * Prefer 640x480 for the first MiniGL fullscreen test. It is the safest
     * classic RTG mode and avoids making the test depend on Workbench depth.
     */
    for (i = 0; i < count; ++i) {
        if (modes[i]->w == 640 && modes[i]->h == 480) {
            chosen = modes[i];
            break;
        }
    }
    if (!chosen && count > 0) {
        chosen = modes[0];
    }

    if (chosen) {
        ww = chosen->w;
        wh = chosen->h;
        printf("SDL fullscreen mode candidate: %dx%d format=%s refresh=%.1f\n",
               chosen->w, chosen->h,
               SDL_GetPixelFormatName(chosen->format),
               chosen->refresh_rate);
    } else {
        printf("No SDL fullscreen modes returned; trying 640x480\n");
    }

    w = SDL_CreateWindow("SDL3 AmigaOS3 Fullscreen MiniGL",
                         ww, wh, SDL_WINDOW_OPENGL);
    if (!w) {
        printf("GL window object failed: %s\n", SDL_GetError());
        SDL_free(modes);
        SDL_Quit();
        return 2;
    }

    if (chosen && !SDL_SetWindowFullscreenMode(w, chosen)) {
        printf("SDL_SetWindowFullscreenMode failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(w);
        SDL_free(modes);
        SDL_Quit();
        return 3;
    }

    if (!SDL_SetWindowFullscreen(w, true)) {
        printf("SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(w);
        SDL_free(modes);
        SDL_Quit();
        return 4;
    }

    printf("Creating MiniGL fullscreen context at %dx%d; backend tries 32-bit then 16-bit\n", ww, wh);

    ctx = SDL_GL_CreateContext(w);
    SDL_free(modes);

    if (!ctx) {
        printf("GL context failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(w);
        SDL_Quit();
        return 5;
    }

    printf("MiniGL fullscreen created. ESC exits.\n");

    while (run) {
        int rw = ww, rh = wh;
        SDL_GetWindowSizeInPixels(w, &rw, &rh);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) run = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) run = false;
        }

        glViewport(0, 0, rw, rh);
        glClearColor(0.05f, 0.1f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBegin(GL_TRIANGLES);
        glColor3f(1,0,0); glVertex2f(-0.7f,-0.6f);
        glColor3f(0,1,0); glVertex2f( 0.7f,-0.6f);
        glColor3f(0,0,1); glVertex2f( 0.0f, 0.7f);
        glEnd();
        SDL_GL_SwapWindow(w);
    }

    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(w);
    SDL_Quit();
    return 0;
}
