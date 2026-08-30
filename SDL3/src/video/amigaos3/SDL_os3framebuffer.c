#include "SDL_internal.h"
#if SDL_VIDEO_DRIVER_AMIGAOS3
#include "SDL_os3framebuffer.h"
bool OS3_CreateWindowFramebuffer(SDL_VideoDevice *_this, SDL_Window *window, Uint32 *format, void **pixels, int *pitch)
{
    SDL_WindowData*d=window->internal;(void)_this;if(!d||!d->syswin)return SDL_SetError("AmigaOS3: no native window");
    if(d->fb_pixels) SDL_free(d->fb_pixels); d->fb_format=SDL_PIXELFORMAT_ARGB8888; d->fb_pitch=window->w*4;
    d->fb_pixels=SDL_calloc(1,(size_t)d->fb_pitch*window->h); if(!d->fb_pixels)return SDL_OutOfMemory();
    *format=d->fb_format;*pixels=d->fb_pixels;*pitch=d->fb_pitch;return true;
}
bool OS3_UpdateWindowFramebuffer(SDL_VideoDevice *_this, SDL_Window *window,const SDL_Rect *rects,int numrects)
{
    SDL_WindowData*d=window->internal;int i;(void)_this;if(!d||!d->syswin||!d->fb_pixels)return false;
    if(!CyberGfxBase)return SDL_SetError("AmigaOS3: RTG framebuffer currently requires CyberGraphX/P96");
    for(i=0;i<numrects;i++){const SDL_Rect*r=&rects[i];WritePixelArray((UBYTE*)d->fb_pixels+r->y*d->fb_pitch+r->x*4,0,0,d->fb_pitch,d->syswin->RPort,r->x+d->syswin->BorderLeft,r->y+d->syswin->BorderTop,r->w,r->h,RECTFMT_ARGB);}
    return true;
}
void OS3_DestroyWindowFramebuffer(SDL_VideoDevice *_this, SDL_Window *window){SDL_WindowData*d=window?window->internal:NULL;(void)_this;if(d&&d->fb_pixels){SDL_free(d->fb_pixels);d->fb_pixels=NULL;}}
#endif
