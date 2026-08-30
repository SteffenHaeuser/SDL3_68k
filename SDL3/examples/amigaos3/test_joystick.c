#include <SDL3/SDL.h>
#include <stdio.h>

int main(void)
{
    SDL_JoystickID *ids;
    int count=0;
    int i;

    if(!SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_EVENTS)){
        printf("SDL_Init failed: %s\n",SDL_GetError());
        return 1;
    }

    ids=SDL_GetJoysticks(&count);
    printf("joysticks: %d\n",count);

    for(i=0;i<count;i++){
        SDL_Joystick *joy=SDL_OpenJoystick(ids[i]);
        if(!joy){
            printf("%d: open failed: %s\n",i,SDL_GetError());
            continue;
        }

        printf("%d: id=%u name=%s path=%s axes=%d buttons=%d hats=%d\n",
               i,
               (unsigned)ids[i],
               SDL_GetJoystickName(joy) ? SDL_GetJoystickName(joy) : "(null)",
               SDL_GetJoystickPath(joy) ? SDL_GetJoystickPath(joy) : "(null)",
               SDL_GetNumJoystickAxes(joy),
               SDL_GetNumJoystickButtons(joy),
               SDL_GetNumJoystickHats(joy));
        SDL_CloseJoystick(joy);
    }

    SDL_free(ids);
    SDL_Quit();
    return 0;
}
