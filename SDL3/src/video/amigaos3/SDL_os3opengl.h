#ifndef SDL_os3opengl_h_
#define SDL_os3opengl_h_
#include "SDL_os3video.h"
bool OS3_GL_LoadLibrary(SDL_VideoDevice *, const char *);
SDL_FunctionPointer OS3_GL_GetProcAddress(SDL_VideoDevice *, const char *);
void OS3_GL_UnloadLibrary(SDL_VideoDevice *);
SDL_GLContext OS3_GL_CreateContext(SDL_VideoDevice *, SDL_Window *);
bool OS3_GL_MakeCurrent(SDL_VideoDevice *, SDL_Window *, SDL_GLContext);
bool OS3_GL_SetSwapInterval(SDL_VideoDevice *, int);
bool OS3_GL_GetSwapInterval(SDL_VideoDevice *, int *);
bool OS3_GL_SwapWindow(SDL_VideoDevice *, SDL_Window *);
bool OS3_GL_DestroyContext(SDL_VideoDevice *, SDL_GLContext);
void OS3_GL_DefaultProfileConfig(SDL_VideoDevice *, int *, int *, int *);
#endif
