#include "SDL_internal.h"

#if SDL_VIDEO_DRIVER_AMIGAOS3

#include "SDL_os3mouse.h"
#include "SDL_os3window.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/default_cursor.h"

#include <devices/inputevent.h>
#include <devices/input.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

struct SDL_CursorData {
    UWORD *sprite_data;
    ULONG alloc_size;
    int width, height;
    int hot_x, hot_y;
    Uint32 colors[3];
};

static SDL_VideoDevice *os3_mouse_device;
static UWORD *hidden_sprite;

typedef struct {
    Uint32 color;
    int count;
} OS3_ColorBucket;

static Uint32 OS3_ColorDistSq(Uint32 a, Uint32 b)
{
    int dr=(int)((a>>16)&255)-(int)((b>>16)&255);
    int dg=(int)((a>>8)&255)-(int)((b>>8)&255);
    int db=(int)(a&255)-(int)(b&255);
    return (Uint32)(dr*dr+dg*dg+db*db);
}

static Uint32 OS3_ReadPixelARGB(SDL_Surface *surface, int x, int y)
{
    Uint8 r=0,g=0,b=0,a=0;
    if (!SDL_ReadSurfacePixel(surface,x,y,&r,&g,&b,&a)) {
        return 0;
    }
    return ((Uint32)a<<24)|((Uint32)r<<16)|((Uint32)g<<8)|b;
}

static int OS3_FindTopColors(SDL_Surface *surface, Uint32 out[3])
{
    OS3_ColorBucket buckets[64];
    int nb=0,x,y,i;
    for (y=0;y<surface->h;y++) {
        for (x=0;x<surface->w;x++) {
            Uint32 px=OS3_ReadPixelARGB(surface,x,y), rgb;
            bool found=false;
            if ((px>>24)<128) continue;
            rgb=px&0x00ffffff;
            for (i=0;i<nb;i++) {
                if (OS3_ColorDistSq(buckets[i].color,rgb)<1024) {
                    buckets[i].count++;
                    found=true;
                    break;
                }
            }
            if (!found && nb<(int)SDL_arraysize(buckets)) {
                buckets[nb].color=rgb;
                buckets[nb].count=1;
                nb++;
            }
        }
    }
    for (i=0;i<nb-1 && i<3;i++) {
        int j,best=i;
        for (j=i+1;j<nb;j++) if (buckets[j].count>buckets[best].count) best=j;
        if (best!=i) { OS3_ColorBucket tmp=buckets[i]; buckets[i]=buckets[best]; buckets[best]=tmp; }
    }
    for (i=0;i<nb && i<3;i++) out[i]=buckets[i].color;
    return SDL_min(nb,3);
}

static int OS3_MapPixel(Uint32 px,const Uint32 pal[3],int n)
{
    int i,best=1;
    Uint32 rgb,dist;
    if ((px>>24)<128 || !n) return 0;
    rgb=px&0x00ffffff; dist=OS3_ColorDistSq(rgb,pal[0]);
    for (i=1;i<n;i++) {
        Uint32 d=OS3_ColorDistSq(rgb,pal[i]);
        if (d<dist) { dist=d; best=i+1; }
    }
    return best;
}

static UWORD *OS3_BuildSprite(SDL_Surface *surface,int hot_x,int hot_y,
                              const Uint32 pal[3],int n,struct SDL_CursorData *d)
{
    int sx=1,sy=1,w=surface->w,h=surface->h,row,col;
    ULONG bytes;
    UWORD *sprite;
    while (w>16) { sx*=2; w=surface->w/sx; }
    while (h>64) { sy*=2; h=surface->h/sy; }
    w=SDL_clamp(w,1,16); h=SDL_clamp(h,1,64);
    bytes=(ULONG)((2+h*2+2)*sizeof(UWORD));
    sprite=(UWORD *)AllocMem(bytes,MEMF_CHIP|MEMF_CLEAR);
    if (!sprite) return NULL;
    for (row=0;row<h;row++) {
        UWORD p0=0,p1=0;
        int py=SDL_min(row*sy,surface->h-1);
        for (col=0;col<w;col++) {
            int px=SDL_min(col*sx,surface->w-1);
            int ci=OS3_MapPixel(OS3_ReadPixelARGB(surface,px,py),pal,n);
            int bit=15-col;
            if (ci&1) p0|=(UWORD)(1U<<bit);
            if (ci&2) p1|=(UWORD)(1U<<bit);
        }
        sprite[2+row*2]=p0; sprite[3+row*2]=p1;
    }
    d->alloc_size=bytes; d->width=w; d->height=h;
    d->hot_x=hot_x/sx; d->hot_y=hot_y/sy;
    return sprite;
}

static void OS3_SetCursorColors(struct Screen *screen,const Uint32 pal[3])
{
    int i;
    if (!screen) return;
    for (i=0;i<3;i++) {
        ULONG r=(pal[i]>>16)&255,g=(pal[i]>>8)&255,b=pal[i]&255;
        SetRGB32(&screen->ViewPort,17+i,
                 (r<<24)|(r<<16)|(r<<8)|r,
                 (g<<24)|(g<<16)|(g<<8)|g,
                 (b<<24)|(b<<16)|(b<<8)|b);
    }
}

static SDL_Cursor *OS3_CreateCursor(SDL_Surface *surface,int hot_x,int hot_y)
{
    SDL_Cursor *c=(SDL_Cursor *)SDL_calloc(1,sizeof(*c));
    struct SDL_CursorData *d=(struct SDL_CursorData *)SDL_calloc(1,sizeof(*d));
    int n;
    if (!c || !d) { SDL_free(c); SDL_free(d); SDL_OutOfMemory(); return NULL; }
    n=OS3_FindTopColors(surface,d->colors);
    if (!n) { n=1; d->colors[0]=0; }
    d->sprite_data=OS3_BuildSprite(surface,hot_x,hot_y,d->colors,n,d);
    if (!d->sprite_data) { SDL_free(d); SDL_free(c); SDL_OutOfMemory(); return NULL; }
    c->internal=d;
    return c;
}

static SDL_Cursor *OS3_CreateSystemCursor(SDL_SystemCursor id)
{
    (void)id;
    return SDL_CreateCursor(default_cdata,default_cmask,
                            DEFAULT_CWIDTH,DEFAULT_CHEIGHT,
                            DEFAULT_CHOTX,DEFAULT_CHOTY);
}

static bool OS3_ShowCursor(SDL_Cursor *cursor)
{
    SDL_Window *w=SDL_GetMouseFocus();
    SDL_WindowData *d=w ? w->internal : NULL;
    if (!d || !d->syswin) return true;
    if (cursor && cursor->internal) {
        struct SDL_CursorData *cd=cursor->internal;
        OS3_SetCursorColors(d->syswin->WScreen,cd->colors);
        SetPointer(d->syswin,cd->sprite_data,(WORD)cd->height,(WORD)cd->width,
                   (WORD)-cd->hot_x,(WORD)-cd->hot_y);
    } else {
        if (!hidden_sprite) hidden_sprite=(UWORD *)AllocMem(6*sizeof(UWORD),MEMF_CHIP|MEMF_CLEAR);
        if (hidden_sprite) SetPointer(d->syswin,hidden_sprite,1,1,0,0);
    }
    return true;
}

static void OS3_FreeCursor(SDL_Cursor *cursor)
{
    struct SDL_CursorData *d;
    if (!cursor) return;
    d=cursor->internal;
    if (d) {
        if (d->sprite_data) FreeMem(d->sprite_data,d->alloc_size);
        SDL_free(d);
    }
    SDL_free(cursor);
}

static bool OS3_WarpMouseInternal(struct Screen *screen,float x,float y)
{
    SDL_VideoData *vd=os3_mouse_device ? os3_mouse_device->internal : NULL;
    struct InputEvent ie;
    struct IEPointerPixel pix;
    if (!vd || !vd->input_req) return SDL_SetError("input.device unavailable for mouse warp");

    SDL_zero(ie); SDL_zero(pix);
    pix.iepp_Screen=screen ? screen : vd->publicScreen;
    pix.iepp_Position.X=(WORD)x; pix.iepp_Position.Y=(WORD)y;
    ie.ie_Class=IECLASS_NEWPOINTERPOS;
    ie.ie_SubClass=IESUBCLASS_PIXEL;
    ie.ie_Code=IECODE_NOBUTTON;
    ie.ie_EventAddress=&pix;
    vd->input_req->io_Data=&ie;
    vd->input_req->io_Length=sizeof(ie);
    vd->input_req->io_Command=IND_WRITEEVENT;
    DoIO((struct IORequest *)vd->input_req);
    return true;
}

static bool OS3_WarpMouse(SDL_Window *window,float x,float y)
{
    SDL_WindowData *d=window ? window->internal : NULL;
    if (!d || !d->syswin) return false;
    if (SDL_GetRelativeMouseMode()) {
        SDL_SendMouseMotion(0,window,SDL_DEFAULT_MOUSE_ID,true,x,y);
        return true;
    }
    return OS3_WarpMouseInternal(d->syswin->WScreen,
        x+d->syswin->LeftEdge+d->syswin->BorderLeft,
        y+d->syswin->TopEdge+d->syswin->BorderTop);
}

static bool OS3_WarpMouseGlobal(float x,float y)
{
    return OS3_WarpMouseInternal(NULL,x,y);
}

static bool OS3_SetRelativeMouseMode(bool enabled)
{
    SDL_Window *w;
    if (!os3_mouse_device) return false;
    for (w=os3_mouse_device->windows;w;w=w->next) {
        SDL_WindowData *d=w->internal;
        if (d && d->syswin) {
            ULONG flags=(w->flags&SDL_WINDOW_FULLSCREEN)?OS3_IDCMP_FULLSCREEN:OS3_IDCMP_WINDOWED;
            if (enabled) flags|=IDCMP_DELTAMOVE;
            ModifyIDCMP(d->syswin,flags);
        }
    }
    return true;
}

void OS3_InitMouse(SDL_VideoDevice *_this)
{
    SDL_Mouse *mouse=SDL_GetMouse();
    os3_mouse_device=_this;
    mouse->CreateCursor=OS3_CreateCursor;
    mouse->CreateSystemCursor=OS3_CreateSystemCursor;
    mouse->ShowCursor=OS3_ShowCursor;
    mouse->FreeCursor=OS3_FreeCursor;
    mouse->WarpMouse=OS3_WarpMouse;
    mouse->WarpMouseGlobal=OS3_WarpMouseGlobal;
    mouse->SetRelativeMouseMode=OS3_SetRelativeMouseMode;
    SDL_SetDefaultCursor(OS3_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
}

void OS3_QuitMouse(SDL_VideoDevice *_this)
{
    (void)_this;
    os3_mouse_device=NULL;
}

void OS3_HandleMouseMotion(SDL_Window *window,WORD x,WORD y)
{
    if (!window) return;
    SDL_SetMouseFocus(window);
    SDL_SendMouseMotion(0,window,SDL_DEFAULT_MOUSE_ID,false,(float)x,(float)y);
}

void OS3_HandleRelativeMouseMotion(SDL_Window *window,WORD dx,WORD dy)
{
    if (!window) return;
    SDL_SetMouseFocus(window);
    SDL_SendMouseMotion(0,window,SDL_DEFAULT_MOUSE_ID,true,(float)dx,(float)dy);
}

void OS3_HandleMouseButton(SDL_Window *window,UWORD code)
{
    UWORD raw; Uint8 button=0; bool down;
    if (!window) return;
    down=(code&IECODE_UP_PREFIX)?false:true;
    raw=code&~IECODE_UP_PREFIX;
    switch(raw) {
    case IECODE_LBUTTON: button=SDL_BUTTON_LEFT; break;
    case IECODE_RBUTTON: button=SDL_BUTTON_RIGHT; break;
    case IECODE_MBUTTON: button=SDL_BUTTON_MIDDLE; break;
#ifdef IECODE_4TH_BUTTON
    case IECODE_4TH_BUTTON: button=SDL_BUTTON_X1; break;
#endif
#ifdef IECODE_5TH_BUTTON
    case IECODE_5TH_BUTTON: button=SDL_BUTTON_X2; break;
#endif
    default: return;
    }
    SDL_SetMouseFocus(window);
    SDL_SendMouseButton(0,window,SDL_DEFAULT_MOUSE_ID,button,down);
}

bool OS3_HandleRawMouseWheel(SDL_Window *window,UWORD code)
{
#if defined(NM_WHEEL_UP) || defined(NM_WHEEL_DOWN) || defined(NM_WHEEL_LEFT) || defined(NM_WHEEL_RIGHT)
    UWORD raw=code&~IECODE_UP_PREFIX;
    if (code&IECODE_UP_PREFIX) return false;
#ifdef NM_WHEEL_UP
    if(raw==NM_WHEEL_UP){SDL_SendMouseWheel(0,window,SDL_DEFAULT_MOUSE_ID,0,1,SDL_MOUSEWHEEL_NORMAL);return true;}
#endif
#ifdef NM_WHEEL_DOWN
    if(raw==NM_WHEEL_DOWN){SDL_SendMouseWheel(0,window,SDL_DEFAULT_MOUSE_ID,0,-1,SDL_MOUSEWHEEL_NORMAL);return true;}
#endif
#ifdef NM_WHEEL_LEFT
    if(raw==NM_WHEEL_LEFT){SDL_SendMouseWheel(0,window,SDL_DEFAULT_MOUSE_ID,1,0,SDL_MOUSEWHEEL_NORMAL);return true;}
#endif
#ifdef NM_WHEEL_RIGHT
    if(raw==NM_WHEEL_RIGHT){SDL_SendMouseWheel(0,window,SDL_DEFAULT_MOUSE_ID,-1,0,SDL_MOUSEWHEEL_NORMAL);return true;}
#endif
#else
    (void)window; (void)code;
#endif
    return false;
}
#endif
