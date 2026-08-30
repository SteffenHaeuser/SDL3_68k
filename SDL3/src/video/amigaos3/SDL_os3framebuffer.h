#ifndef SDL_os3framebuffer_h_
#define SDL_os3framebuffer_h_
#include "SDL_os3video.h"
bool OS3_CreateWindowFramebuffer(SDL_VideoDevice *, SDL_Window *, Uint32 *, void **, int *);
bool OS3_UpdateWindowFramebuffer(SDL_VideoDevice *, SDL_Window *, const SDL_Rect *, int);
void OS3_DestroyWindowFramebuffer(SDL_VideoDevice *, SDL_Window *);
#endif
