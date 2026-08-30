#include <SDL3/SDL.h>
#include <stdio.h>

static volatile bool done=false;

static void cb(void *u,const char * const *files,int filter)
{
    int i;
    (void)u;
    printf("selected filter index: %d\n",filter);
    if(!files) printf("dialog error: %s\n",SDL_GetError());
    else if(!files[0]) printf("cancelled\n");
    else for(i=0;files[i];i++) printf("selected: %s\n",files[i]);
    done=true;
}

int main(void)
{
    static const SDL_DialogFileFilter filters[]={
        {"Images","png;jpg;jpeg;bmp"},
        {"Text","txt;md"}
    };

    if(!SDL_Init(SDL_INIT_VIDEO)){
        printf("init: %s\n",SDL_GetError());
        return 1;
    }

    puts("ASL should show only image/text extensions via its pattern field.");
    puts("Classic ASL has no SDL-style selectable named filter popup; callback filter stays -1.");
    SDL_ShowOpenFileDialog(cb,NULL,NULL,filters,SDL_arraysize(filters),"PROGDIR:",true);
    while(!done){
        SDL_PumpEvents();
        SDL_Delay(20);
    }
    SDL_Quit();
    return 0;
}
