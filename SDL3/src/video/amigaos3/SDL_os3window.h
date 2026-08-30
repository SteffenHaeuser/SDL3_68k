#ifndef SDL_os3window_h_
#define SDL_os3window_h_
#include "SDL_os3video.h"
#define OS3_IDCMP_WINDOWED (IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE|IDCMP_NEWSIZE|IDCMP_CHANGEWINDOW|IDCMP_ACTIVEWINDOW|IDCMP_INACTIVEWINDOW)
#define OS3_IDCMP_FULLSCREEN (IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE|IDCMP_ACTIVEWINDOW|IDCMP_INACTIVEWINDOW)
bool OS3_CreateWindow(SDL_VideoDevice *, SDL_Window *, SDL_PropertiesID);
void OS3_DestroyWindow(SDL_VideoDevice *, SDL_Window *);
void OS3_SetWindowTitle(SDL_VideoDevice *, SDL_Window *);
void OS3_ShowWindow(SDL_VideoDevice *, SDL_Window *);
void OS3_HideWindow(SDL_VideoDevice *, SDL_Window *);
void OS3_RaiseWindow(SDL_VideoDevice *, SDL_Window *);
bool OS3_SetWindowPosition(SDL_VideoDevice *, SDL_Window *);
void OS3_SetWindowSize(SDL_VideoDevice *, SDL_Window *);
void OS3_SetWindowMinMaxSize(SDL_VideoDevice *, SDL_Window *);
bool OS3_GetWindowBordersSize(SDL_VideoDevice *, SDL_Window *, int *, int *, int *, int *);
void OS3_MaximizeWindow(SDL_VideoDevice *, SDL_Window *);
void OS3_MinimizeWindow(SDL_VideoDevice *, SDL_Window *);
void OS3_RestoreWindow(SDL_VideoDevice *, SDL_Window *);
void OS3_SetWindowBordered(SDL_VideoDevice *, SDL_Window *, bool);
void OS3_SetWindowResizable(SDL_VideoDevice *, SDL_Window *, bool);
void OS3_SetWindowAlwaysOnTop(SDL_VideoDevice *, SDL_Window *, bool);
bool OS3_SetWindowMouseGrab(SDL_VideoDevice *, SDL_Window *, bool);
bool OS3_SetWindowKeyboardGrab(SDL_VideoDevice *, SDL_Window *, bool);
SDL_FullscreenResult OS3_SetWindowFullscreen(SDL_VideoDevice *, SDL_Window *, SDL_VideoDisplay *, SDL_FullscreenOp);
#endif
