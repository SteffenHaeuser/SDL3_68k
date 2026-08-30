#ifndef SDL_os3keyboard_h_
#define SDL_os3keyboard_h_
#include "SDL_os3video.h"
void OS3_InitKeyboard(SDL_VideoDevice *_this);
void OS3_QuitKeyboard(SDL_VideoDevice *_this);
void OS3_HandleRawKey(SDL_VideoDevice *_this, SDL_Window *window, UWORD code, UWORD qualifier);
int OS3_TranslateRawKey(UWORD code, UWORD qualifier, APTR iaddress, char *utf8, int utf8_size);
bool OS3_StartTextInput(SDL_VideoDevice *_this, SDL_Window *window, SDL_PropertiesID props);
bool OS3_StopTextInput(SDL_VideoDevice *_this, SDL_Window *window);
#endif
