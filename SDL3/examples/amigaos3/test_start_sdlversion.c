
#include <proto/dos.h>
#include <SDL3/SDL_version.h>

int main(void)
{
    int v;
    PutStr("start_sdlversion: entered main()\n");
    PutStr("start_sdlversion: before SDL_GetVersion()\n");
    v = SDL_GetVersion();
    (void)v;
    PutStr("start_sdlversion: after SDL_GetVersion()\n");
    return 0;
}
