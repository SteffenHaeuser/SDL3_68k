#ifndef SDL_os3mouse_h_
#define SDL_os3mouse_h_
#include "SDL_os3video.h"
void OS3_InitMouse(SDL_VideoDevice *_this);
void OS3_QuitMouse(SDL_VideoDevice *_this);
void OS3_HandleMouseMotion(SDL_Window *window, WORD x, WORD y);
void OS3_HandleRelativeMouseMotion(SDL_Window *window, WORD dx, WORD dy);
void OS3_HandleMouseButton(SDL_Window *window, UWORD code);
bool OS3_HandleRawMouseWheel(SDL_Window *window, UWORD code);
#endif
