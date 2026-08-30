#include "SDL_internal.h"

#if SDL_VIDEO_DRIVER_AMIGAOS3

#include "SDL_os3modes.h"

#include <graphics/displayinfo.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

static SDL_PixelFormat OS3_FormatForDepth(int depth)
{
    if (depth <= 8) {
        return SDL_PIXELFORMAT_INDEX8;
    } else if (depth <= 15) {
        return SDL_PIXELFORMAT_XRGB1555;
    } else if (depth <= 16) {
        return SDL_PIXELFORMAT_RGB565;
    } else if (depth <= 24) {
        return SDL_PIXELFORMAT_XRGB8888;
    }
    return SDL_PIXELFORMAT_ARGB8888;
}

static bool OS3_GetDisplayMode(ULONG modeid, SDL_DisplayMode *mode)
{
    APTR handle;
    struct DimensionInfo dims;
    struct DisplayInfo disp;
    SDL_DisplayModeData *data;

    handle = FindDisplayInfo(modeid);
    if (!handle) {
        return false;
    }

    if (!GetDisplayInfoData(handle, (UBYTE *)&dims, sizeof(dims), DTAG_DIMS, 0)) {
        return false;
    }
    if (!GetDisplayInfoData(handle, (UBYTE *)&disp, sizeof(disp), DTAG_DISP, 0)) {
        return false;
    }

    /* SDL3/AmigaOS3 is intended for RTG fullscreen modes. */
#ifdef DIPF_IS_RTG
    if (!(disp.PropertyFlags & DIPF_IS_RTG)) {
        return false;
    }
#endif

    data = (SDL_DisplayModeData *)SDL_calloc(1, sizeof(*data));
    if (!data) {
        return false;
    }

    data->modeid = modeid;
    data->depth = (int)dims.MaxDepth;

    SDL_zero(*mode);
    mode->w = dims.Nominal.MaxX - dims.Nominal.MinX + 1;
    mode->h = dims.Nominal.MaxY - dims.Nominal.MinY + 1;
    mode->pixel_density = 1.0f;
    mode->refresh_rate = 60.0f;
    mode->format = OS3_FormatForDepth(data->depth);
    mode->internal = data;
    return true;
}

bool OS3_GetDisplayBounds(SDL_VideoDevice *_this, SDL_VideoDisplay *display, SDL_Rect *rect)
{
    (void)_this;
    if (!display || !display->current_mode || !rect) {
        return false;
    }

    rect->x = 0;
    rect->y = 0;
    rect->w = display->current_mode->w;
    rect->h = display->current_mode->h;
    return true;
}

bool OS3_GetDisplayModes(SDL_VideoDevice *_this, SDL_VideoDisplay *display)
{
    ULONG id = INVALID_ID;
    (void)_this;

    while ((id = NextDisplayInfo(id)) != INVALID_ID) {
        SDL_DisplayMode mode;

        if (OS3_GetDisplayMode(id, &mode)) {
            if (!SDL_AddFullscreenDisplayMode(display, &mode)) {
                SDL_free(mode.internal);
            }
        }
    }

    return true;
}

bool OS3_SetDisplayMode(SDL_VideoDevice *_this, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    SDL_DisplayData *dd;
    SDL_DisplayModeData *md;

    (void)_this;

    if (!display || !display->internal) {
        return SDL_SetError("AmigaOS3: invalid display");
    }

    dd = (SDL_DisplayData *)display->internal;

    /*
     * SDL calls SetDisplayMode before SetWindowFullscreen. Do not open the
     * native Screen here: for software windows SetWindowFullscreen owns the
     * Screen transition, while MiniGL creates/owns its own fullscreen Screen.
     */
    if (!mode || mode == &display->desktop_mode) {
        dd->selected_modeid = dd->desktop_modeid;
        dd->selected_depth = dd->desktop_depth;
        dd->selected_w = display->desktop_mode.w;
        dd->selected_h = display->desktop_mode.h;
        return true;
    }

    md = (SDL_DisplayModeData *)mode->internal;
    if (!md) {
        return SDL_SetError("AmigaOS3: fullscreen mode has no native mode id");
    }

    dd->selected_modeid = md->modeid;
    dd->selected_depth = md->depth;
    dd->selected_w = mode->w;
    dd->selected_h = mode->h;
    return true;
}

struct Screen *OS3_OpenFullscreenScreen(SDL_VideoDevice *_this, SDL_VideoDisplay *display)
{
    SDL_DisplayData *dd;
    struct Screen *screen;
    ULONG error = 0;
    ULONG modeid;
    int depth;
    int w, h;

    if (!display || !display->internal) {
        SDL_SetError("AmigaOS3: invalid display for fullscreen");
        return NULL;
    }

    dd = (SDL_DisplayData *)display->internal;
    if (dd->screen) {
        return dd->screen;
    }

    modeid = dd->selected_modeid ? dd->selected_modeid : dd->desktop_modeid;
    depth = dd->selected_depth ? dd->selected_depth : dd->desktop_depth;
    w = dd->selected_w ? dd->selected_w : display->desktop_mode.w;
    h = dd->selected_h ? dd->selected_h : display->desktop_mode.h;

    screen = OpenScreenTags(NULL,
        SA_Type, CUSTOMSCREEN,
        SA_DisplayID, modeid,
        SA_Width, w,
        SA_Height, h,
        SA_Depth, depth,
        SA_Quiet, TRUE,
        SA_ShowTitle, FALSE,
        SA_AutoScroll, FALSE,
        SA_ErrorCode, (ULONG)&error,
        TAG_DONE);

    if (!screen) {
        SDL_SetError("AmigaOS3: OpenScreenTags failed for mode 0x%08lx (%dx%dx%d), error %lu",
                     (unsigned long)modeid, w, h, depth, (unsigned long)error);
        return NULL;
    }

    SetRast(&screen->RastPort, 0);
    dd->screen = screen;
    return screen;
}

void OS3_CloseFullscreenScreen(SDL_VideoDevice *_this, SDL_VideoDisplay *display)
{
    SDL_DisplayData *dd;
    (void)_this;

    if (!display || !display->internal) {
        return;
    }

    dd = (SDL_DisplayData *)display->internal;
    if (dd->screen) {
        CloseScreen(dd->screen);
        dd->screen = NULL;
    }
}

#endif
