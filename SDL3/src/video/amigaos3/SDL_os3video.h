#include "SDL_internal.h"
#ifndef SDL_os3video_h_
#define SDL_os3video_h_
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/displayinfo.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <devices/input.h>
#include "../SDL_sysvideo.h"

typedef struct SDL_VideoData {
    struct Screen *publicScreen;
    struct MsgPort *input_port;
    struct IOStdReq *input_req;
} SDL_VideoData;

typedef struct SDL_DisplayData {
    struct Screen *screen;
    ULONG desktop_modeid;
    ULONG selected_modeid;
    int desktop_depth;
    int selected_depth;
    int selected_w;
    int selected_h;
} SDL_DisplayData;

typedef struct SDL_DisplayModeData {
    ULONG modeid;
    int depth;
} SDL_DisplayModeData;

struct SDL_WindowData {
    SDL_Window *sdlwin;
    struct Window *syswin;
    struct Screen *screen;
    bool owns_screen;
    SDL_GLContext gl_context;
    void *fb_pixels;
    int fb_pitch;
    Uint32 fb_format;
};

extern struct Library *CyberGfxBase;
extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;
extern struct Library *KeymapBase;
extern VideoBootStrap AMIGAOS3_bootstrap;
#endif
