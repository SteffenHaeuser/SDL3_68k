#include "SDL_internal.h"
#if SDL_VIDEO_DRIVER_AMIGAOS3
#include "SDL_os3video.h"
#include "SDL_os3window.h"
#include "SDL_os3framebuffer.h"
#include "SDL_os3events.h"
#include "SDL_os3keyboard.h"
#include "SDL_os3mouse.h"
#include "SDL_os3modes.h"
#include "SDL_os3messagebox.h"
#include <proto/keymap.h>
#if !defined(SDL_AMIGAOS3_SW_ONLY)
#include "SDL_os3opengl.h"
#endif

struct Library *CyberGfxBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Library *KeymapBase = NULL;

static bool OS3_VideoInit(SDL_VideoDevice *_this)
{
    SDL_VideoData *data=(SDL_VideoData *)_this->internal;
    SDL_VideoDisplay display;
    SDL_DisplayMode mode;
    SDL_DisplayData *displaydata;
    SDL_DisplayModeData *modedata;
    struct BitMap *bm;
    ULONG modeid;
    ULONG w=640,h=480,d=32;
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    if (!GfxBase) return SDL_SetError("AmigaOS3: cannot open graphics.library V39+");
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    if (!IntuitionBase) return SDL_SetError("AmigaOS3: cannot open intuition.library V39+");
    KeymapBase=OpenLibrary("keymap.library",37);
    if (!KeymapBase) return SDL_SetError("AmigaOS3: cannot open keymap.library V37+");

    data->input_port=CreateMsgPort();
    if (data->input_port) {
        data->input_req=(struct IOStdReq *)CreateIORequest(data->input_port,sizeof(struct IOStdReq));
        if (data->input_req) {
            if (OpenDevice("input.device",0,(struct IORequest *)data->input_req,0) != 0) {
                DeleteIORequest((struct IORequest *)data->input_req);
                data->input_req=NULL;
            }
        }
    }
    CyberGfxBase=OpenLibrary("cybergraphics.library",40);
    if (!CyberGfxBase) CyberGfxBase=OpenLibrary("Picasso96API.library",0);
    data->publicScreen=LockPubScreen(NULL);
    if (!data->publicScreen) return SDL_SetError("AmigaOS3: cannot lock public screen");
    bm=data->publicScreen->RastPort.BitMap;
    w=data->publicScreen->Width; h=data->publicScreen->Height;
    if (CyberGfxBase && bm) d=GetCyberMapAttr(bm,CYBRMATTR_DEPTH);
    modeid=GetVPModeID(&data->publicScreen->ViewPort);

    displaydata=(SDL_DisplayData *)SDL_calloc(1,sizeof(*displaydata));
    modedata=(SDL_DisplayModeData *)SDL_calloc(1,sizeof(*modedata));
    if (!displaydata || !modedata) {
        SDL_free(displaydata); SDL_free(modedata);
        return SDL_OutOfMemory();
    }

    modedata->modeid=modeid;
    modedata->depth=(int)d;
    displaydata->desktop_modeid=modeid;
    displaydata->selected_modeid=modeid;
    displaydata->desktop_depth=(int)d;
    displaydata->selected_depth=(int)d;
    displaydata->selected_w=(int)w;
    displaydata->selected_h=(int)h;

    SDL_zero(mode); mode.w=(int)w; mode.h=(int)h; mode.pixel_density=1.0f; mode.refresh_rate=60.0f;
    mode.format=(d<=8)?SDL_PIXELFORMAT_INDEX8:((d<=16)?SDL_PIXELFORMAT_RGB565:SDL_PIXELFORMAT_ARGB8888);
    mode.internal=modedata;

    SDL_zero(display);
    display.desktop_mode=mode;
    display.current_mode=&display.desktop_mode;
    display.internal=displaydata;
    if (!SDL_AddVideoDisplay(&display,false)) {
        SDL_free(displaydata); SDL_free(modedata);
        return false;
    }
    OS3_InitKeyboard(_this);
    OS3_InitMouse(_this);
    return true;
}
static void OS3_VideoQuit(SDL_VideoDevice *_this)
{
    SDL_VideoData *data=(SDL_VideoData *)_this->internal;
    OS3_QuitMouse(_this);
    OS3_QuitKeyboard(_this);
    if (_this->num_displays > 0 && _this->displays[0]) {
        OS3_CloseFullscreenScreen(_this, _this->displays[0]);
    }
    if (data && data->publicScreen) { UnlockPubScreen(NULL,data->publicScreen); data->publicScreen=NULL; }
    if (data && data->input_req) { CloseDevice((struct IORequest *)data->input_req); DeleteIORequest((struct IORequest *)data->input_req); data->input_req=NULL; }
    if (data && data->input_port) { DeleteMsgPort(data->input_port); data->input_port=NULL; }
    if (KeymapBase) { CloseLibrary(KeymapBase); KeymapBase=NULL; }
    if (CyberGfxBase) { CloseLibrary(CyberGfxBase); CyberGfxBase=NULL; }
    if (IntuitionBase) { CloseLibrary((struct Library *)IntuitionBase); IntuitionBase=NULL; }
    if (GfxBase) { CloseLibrary((struct Library *)GfxBase); GfxBase=NULL; }
}
static void OS3_DeleteDevice(SDL_VideoDevice *device) { if (device) { SDL_free(device->internal); SDL_free(device); } }
static SDL_VideoDevice *OS3_CreateDevice(void)
{
    SDL_VideoDevice *device=(SDL_VideoDevice *)SDL_calloc(1,sizeof(*device));
    SDL_VideoData *data=(SDL_VideoData *)SDL_calloc(1,sizeof(*data));
    if (!device || !data) { SDL_free(device); SDL_free(data); SDL_OutOfMemory(); return NULL; }
    device->internal=data; device->VideoInit=OS3_VideoInit; device->VideoQuit=OS3_VideoQuit; device->free=OS3_DeleteDevice;
    device->GetDisplayBounds=OS3_GetDisplayBounds;
    device->GetDisplayModes=OS3_GetDisplayModes;
    device->SetDisplayMode=OS3_SetDisplayMode;
    device->CreateSDLWindow=OS3_CreateWindow; device->DestroyWindow=OS3_DestroyWindow;
    device->SetWindowTitle=OS3_SetWindowTitle;
    device->SetWindowPosition=OS3_SetWindowPosition;
    device->SetWindowSize=OS3_SetWindowSize;
    device->SetWindowMinimumSize=OS3_SetWindowMinMaxSize;
    device->SetWindowMaximumSize=OS3_SetWindowMinMaxSize;
    device->GetWindowBordersSize=OS3_GetWindowBordersSize;
    device->ShowWindow=OS3_ShowWindow;
    device->HideWindow=OS3_HideWindow;
    device->RaiseWindow=OS3_RaiseWindow;
    device->MaximizeWindow=OS3_MaximizeWindow;
    device->MinimizeWindow=OS3_MinimizeWindow;
    device->RestoreWindow=OS3_RestoreWindow;
    device->SetWindowBordered=OS3_SetWindowBordered;
    device->SetWindowResizable=OS3_SetWindowResizable;
    device->SetWindowAlwaysOnTop=OS3_SetWindowAlwaysOnTop;
    device->SetWindowMouseGrab=OS3_SetWindowMouseGrab;
    device->SetWindowKeyboardGrab=OS3_SetWindowKeyboardGrab;
    device->SetWindowFullscreen=OS3_SetWindowFullscreen;
    device->CreateWindowFramebuffer=OS3_CreateWindowFramebuffer; device->UpdateWindowFramebuffer=OS3_UpdateWindowFramebuffer; device->DestroyWindowFramebuffer=OS3_DestroyWindowFramebuffer;
    /* AmigaOS3 only has the software renderer for SDL_Renderer.
     * Do not let SDL3 try its texture-framebuffer acceleration probe:
     *
     * SDL_CreateRenderer("software")
     *   -> SW_CreateRenderer()
     *   -> SDL_GetWindowSurface()
     *   -> SDL_CreateWindowFramebuffer()
     *
     * Without this flag SDL_GetWindowSurface() first tries to create an
     * accelerated renderer-backed framebuffer, which recursively enters
     * SDL_CreateRenderer() again.  The SDL2 AmigaOS3 backend needs the
     * same workaround.
     */
    device->checked_texture_framebuffer = true;
    device->PumpEvents=OS3_PumpEvents;
    device->StartTextInput=OS3_StartTextInput; device->StopTextInput=OS3_StopTextInput;
#if !defined(SDL_AMIGAOS3_SW_ONLY)
    device->GL_LoadLibrary=OS3_GL_LoadLibrary; device->GL_GetProcAddress=OS3_GL_GetProcAddress; device->GL_UnloadLibrary=OS3_GL_UnloadLibrary;
    device->GL_CreateContext=OS3_GL_CreateContext; device->GL_MakeCurrent=OS3_GL_MakeCurrent; device->GL_SetSwapInterval=OS3_GL_SetSwapInterval;
    device->GL_GetSwapInterval=OS3_GL_GetSwapInterval; device->GL_SwapWindow=OS3_GL_SwapWindow; device->GL_DestroyContext=OS3_GL_DestroyContext;
    device->GL_DefaultProfileConfig=OS3_GL_DefaultProfileConfig;
#endif
    return device;
}
VideoBootStrap AMIGAOS3_bootstrap={"amigaos3","SDL AmigaOS 3.x CyberGraphX/P96 video driver",OS3_CreateDevice,OS3_ShowMessageBox,false};
#endif
