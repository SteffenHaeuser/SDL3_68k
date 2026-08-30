#include <SDL3/SDL.h>
#include <proto/dos.h>
#include <stdio.h>

int main(void)
{
    SDL_Window *w;
    SDL_Event e;
    bool run=true, relative=false;

    PutStr("input: SDL_Init\n");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init failed: %s\n",SDL_GetError()); return 1;
    }
    w=SDL_CreateWindow("SDL3 AmigaOS3 Input Test",480,320,0);
    if (!w) { printf("window failed: %s\n",SDL_GetError()); return 2; }

    SDL_StartTextInput(w);
    PutStr("Keys/mouse generate events. R toggles relative mode, W warps to center, ESC quits.\n");

    while(run) {
        while(SDL_PollEvent(&e)) {
            switch(e.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: run=false; break;
            case SDL_EVENT_KEY_DOWN:
                printf("key down scancode=%d key=%ld\n",(int)e.key.scancode,(long)e.key.key);
                if(e.key.key==SDLK_ESCAPE) run=false;
                if(e.key.key==SDLK_R) {
                    relative=!relative;
                    if(!SDL_SetWindowRelativeMouseMode(w,relative))
                        printf("relative mode failed: %s\n",SDL_GetError());
                    else printf("relative mode: %s\n",relative?"ON":"OFF");
                }
                if(e.key.key==SDLK_W) {
                    SDL_WarpMouseInWindow(w,240,160);
                    printf("warp requested\n");
                }
                break;
            case SDL_EVENT_KEY_UP:
                printf("key up scancode=%d\n",(int)e.key.scancode); break;
            case SDL_EVENT_TEXT_INPUT:
                printf("text: '%s'\n",e.text.text); break;
            case SDL_EVENT_MOUSE_MOTION:
                printf("mouse %s x=%.0f y=%.0f xrel=%.0f yrel=%.0f\n",
                       relative?"relative":"absolute",e.motion.x,e.motion.y,e.motion.xrel,e.motion.yrel); break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                printf("mouse button %u %s\n",(unsigned)e.button.button,
                       e.type==SDL_EVENT_MOUSE_BUTTON_DOWN?"down":"up"); break;
            case SDL_EVENT_MOUSE_WHEEL:
                printf("wheel %.0f %.0f\n",e.wheel.x,e.wheel.y); break;
            default: break;
            }
        }
        SDL_Delay(10);
    }
    SDL_StopTextInput(w);
    SDL_DestroyWindow(w);
    SDL_Quit();
    return 0;
}
