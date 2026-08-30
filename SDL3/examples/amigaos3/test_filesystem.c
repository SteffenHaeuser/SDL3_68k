#include <SDL3/SDL.h>
#include <stdio.h>

static SDL_EnumerationResult enumcb(void *u,const char *dir,const char *name)
{
    (void)u;
    printf("  %s%s\n",dir,name);
    return SDL_ENUM_CONTINUE;
}

int main(void)
{
    char *base=SDL_GetBasePath();
    char *cwd=SDL_GetCurrentDirectory();
    char *pref=SDL_GetPrefPath("SDL3Test","AmigaOS3");
    SDL_PathInfo pi;

    printf("base: %s\n",base?base:"(null)");
    printf("cwd: %s\n",cwd?cwd:"(null)");
    printf("pref: %s\n",pref?pref:"(null)");

    printf("PROGDIR enumeration:\n");
    if(!SDL_EnumerateDirectory("PROGDIR:",enumcb,NULL))
        printf("enum error: %s\n",SDL_GetError());

    if(SDL_GetPathInfo("PROGDIR:",&pi)){
        SDL_DateTime dt;
        printf("PROGDIR type=%d size=%llu modify_ns=%lld\n",
               (int)pi.type,(unsigned long long)pi.size,(long long)pi.modify_time);
        if(pi.modify_time && SDL_TimeToDateTime(pi.modify_time,&dt,true)){
            printf("modify local: %04d-%02d-%02d %02d:%02d:%02d offset=%d\n",
                   dt.year,dt.month,dt.day,dt.hour,dt.minute,dt.second,dt.utc_offset);
        }
    } else {
        printf("path info error: %s\n",SDL_GetError());
    }

    SDL_free(base);
    SDL_free(cwd);
    SDL_free(pref);
    return 0;
}
