#ifndef SDL_os3modes_h_
#define SDL_os3modes_h_

#include "SDL_os3video.h"

bool OS3_GetDisplayBounds(SDL_VideoDevice *_this, SDL_VideoDisplay *display, SDL_Rect *rect);
bool OS3_GetDisplayModes(SDL_VideoDevice *_this, SDL_VideoDisplay *display);
bool OS3_SetDisplayMode(SDL_VideoDevice *_this, SDL_VideoDisplay *display, SDL_DisplayMode *mode);

struct Screen *OS3_OpenFullscreenScreen(SDL_VideoDevice *_this, SDL_VideoDisplay *display);
void OS3_CloseFullscreenScreen(SDL_VideoDevice *_this, SDL_VideoDisplay *display);

#endif
