#include <SDL3/SDL.h>
#include <stdio.h>

int main(void)
{
    SDL_Window *w;
    SDL_Event e;
    bool run=true;

    if(!SDL_Init(SDL_INIT_VIDEO)){printf("init: %s\n",SDL_GetError());return 1;}
    w=SDL_CreateWindow("SDL3 OS3 Window API",400,260,SDL_WINDOW_RESIZABLE);
    if(!w){printf("window: %s\n",SDL_GetError());return 2;}

    SDL_SetWindowMinimumSize(w,240,160);
    SDL_SetWindowMaximumSize(w,700,500);
    printf("Keys: P=position S=size M=maximize N=minimize R=restore B=border Q=resizable ESC=quit\n");

    while(run){
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_EVENT_QUIT||e.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED)run=false;
            if(e.type==SDL_EVENT_KEY_DOWN){
                switch(e.key.key){
                case SDLK_ESCAPE:run=false;break;
                case SDLK_P: SDL_SetWindowPosition(w,80,80); break;
                case SDLK_S: SDL_SetWindowSize(w,520,340); break;
                case SDLK_M: SDL_MaximizeWindow(w); break;
                case SDLK_N: SDL_MinimizeWindow(w); break;
                case SDLK_R: SDL_RestoreWindow(w); break;
                case SDLK_B: SDL_SetWindowBordered(w,(SDL_GetWindowFlags(w)&SDL_WINDOW_BORDERLESS)!=0); break;
                case SDLK_Q: SDL_SetWindowResizable(w,(SDL_GetWindowFlags(w)&SDL_WINDOW_RESIZABLE)==0); break;
                default:break;
                }
            }
        }
        SDL_Delay(10);
    }
    SDL_DestroyWindow(w);SDL_Quit();return 0;
}
