#include "SDL_internal.h"
#if SDL_VIDEO_DRIVER_AMIGAOS3 && defined(SDL_VIDEO_OPENGL)
#include <stddef.h>
#include <proto/minigl.h>
#include "SDL_os3opengl.h"
#include "SDL_os3window.h"

static bool opened = false;
static int swap_interval = 0;

static void select_ctx(SDL_GLContext c)
{
    if (MiniGLDispatch && MiniGLDispatch->currentContext)
        *MiniGLDispatch->currentContext = (GLcontext)c;
}

void OS3_GL_DefaultProfileConfig(SDL_VideoDevice *_this, int *mask, int *major, int *minor)
{
    (void)_this;
    if (mask) *mask = 0;
    if (major) *major = 1;
    if (minor) *minor = 2;
}

bool OS3_GL_LoadLibrary(SDL_VideoDevice *_this, const char *path)
{
    (void)_this;
    (void)path;
    if (opened) return true;
    if (!MiniGLOpen()) return SDL_SetError("AmigaOS3: cannot open minigl.library");
    if (!MiniGLDispatch ||
        MiniGLDispatch->abiVersion != MINIGL_DISPATCH_ABI_VERSION ||
        MiniGLDispatch->structSize < offsetof(MGLDispatchTable, MGLCreateContextFromWindow) +
                                    sizeof(MiniGLDispatch->MGLCreateContextFromWindow) ||
        !MiniGLDispatch->currentContext || !MiniGLDispatch->MGLCreateContextFromWindow) {
        MiniGLClose();
        return SDL_SetError("AmigaOS3: minigl.library lacks the v27.2 FromWindow dispatch entry");
    }
    opened = true;
    return true;
}

void OS3_GL_UnloadLibrary(SDL_VideoDevice *_this)
{
    (void)_this;
    if (opened) {
        MiniGLClose();
        opened = false;
    }
}

SDL_GLContext OS3_GL_CreateContext(SDL_VideoDevice *_this, SDL_Window *window)
{
    SDL_WindowData *d = window ? window->internal : NULL;
    SDL_GLContext c;
    struct Window *w;
    int depth, width, height;

    if (!d || !d->syswin) {
        SDL_SetError("AmigaOS3: SDL window has no native drawable");
        return NULL;
    }
    if (d->gl_context) {
        SDL_SetError("AmigaOS3: this window already has a MiniGL context");
        return NULL;
    }
    if (!opened && !OS3_GL_LoadLibrary(_this, NULL)) return NULL;
    w = d->syswin;
    if (!w->RPort || !w->RPort->BitMap || !CyberGfxBase) {
        SDL_SetError("AmigaOS3: MiniGL requires an RTG drawable");
        return NULL;
    }
    depth = GetBitMapAttr(w->RPort->BitMap, BMA_DEPTH);
    width = w->Width - w->BorderLeft - w->BorderRight;
    height = w->Height - w->BorderTop - w->BorderBottom;
    if (depth <= 8 || width <= 0 || height <= 0) {
        SDL_SetError("AmigaOS3: MiniGL requires a nonempty true-color window");
        return NULL;
    }
    mglChooseNumberOfBuffers(_this->gl_config.double_buffer ? 2 : 1);
    mglChoosePixelDepth(depth <= 16 ? 16 : 32);
    /* Both SDL-created and application-created native windows are borrowed. */
    c = (SDL_GLContext)mglCreateContextFromWindow(w);
    if (!c) {
        SDL_SetError("AmigaOS3: mglCreateContextFromWindow failed");
        return NULL;
    }
    d->gl_context = c;
    d->gl_width = width;
    d->gl_height = height;
    select_ctx(c);
    return c;
}

void OS3_GL_ResizeWindow(SDL_VideoDevice *_this, SDL_Window *window, int width, int height)
{
    SDL_WindowData *d = window ? window->internal : NULL;
    GLcontext previous;
    (void)_this;
    if (!d || !d->gl_context || width <= 0 || height <= 0) return;
    if (width == d->gl_width && height == d->gl_height) return;
    previous = *MiniGLDispatch->currentContext;
    select_ctx(d->gl_context);
    mglResizeContext(width, height);
    d->gl_width = width;
    d->gl_height = height;
    *MiniGLDispatch->currentContext = previous;
}

bool OS3_GL_MakeCurrent(SDL_VideoDevice *_this, SDL_Window *window, SDL_GLContext c)
{
    SDL_WindowData *d;
    if (!c) { select_ctx(NULL); return true; }
    if (!window || !window->internal)
        return SDL_SetError("AmigaOS3: MiniGL context requires its SDL window");
    d = window->internal;
    if (d->gl_context != c)
        return SDL_SetError("AmigaOS3: context belongs to a different SDL window");
    OS3_GetWindowSizeInPixels(_this, window, NULL, NULL);
    select_ctx(c);
    return true;
}

bool OS3_GL_SetSwapInterval(SDL_VideoDevice *_this, int interval)
{
    (void)_this;
    if (interval != 0 && interval != 1)
        return SDL_SetError("AmigaOS3: swap interval supports only 0/1");
    swap_interval = interval;
    return true;
}

bool OS3_GL_GetSwapInterval(SDL_VideoDevice *_this, int *interval)
{
    (void)_this;
    if (interval) *interval = swap_interval;
    return true;
}

bool OS3_GL_SwapWindow(SDL_VideoDevice *_this, SDL_Window *window)
{
    SDL_WindowData *d = window ? window->internal : NULL;
    if (!d || !d->gl_context) return SDL_SetError("AmigaOS3: no MiniGL context");
    OS3_GetWindowSizeInPixels(_this, window, NULL, NULL);
    select_ctx(d->gl_context);
    mglSwitchDisplay();
    return true;
}

bool OS3_GL_DestroyContext(SDL_VideoDevice *_this, SDL_GLContext c)
{
    SDL_Window *window;
    GLcontext previous;
    if (!c) return true;
    previous = *MiniGLDispatch->currentContext;
    for (window = _this->windows; window; window = window->next) {
        SDL_WindowData *d = window->internal;
        if (d && d->gl_context == c) {
            d->gl_context = NULL;
            d->gl_width = d->gl_height = 0;
            break;
        }
    }
    select_ctx(c);
    mglDeleteContext();
    select_ctx(previous == (GLcontext)c ? NULL : (SDL_GLContext)previous);
    /* Native window, screen and event subscriptions remain intact. */
    return true;
}

/*
 * The MiniGL SDK exposes GL entry points as static inline dispatch wrappers.
 * Their addresses are therefore stable callable wrappers which dispatch
 * through the current minigl.library context.
 */
typedef struct OS3_GLProcEntry {
    const char *name;
    SDL_FunctionPointer address;
} OS3_GLProcEntry;

#define OS3_GLPROC(fn) { #fn, (SDL_FunctionPointer)(fn) }

static const OS3_GLProcEntry os3_gl_procs[] = {
    OS3_GLPROC(glActiveTextureARB),
    OS3_GLPROC(glAlphaFunc),
    OS3_GLPROC(glArrayElement),
    OS3_GLPROC(glBegin),
    OS3_GLPROC(glBindTexture),
    OS3_GLPROC(glBlendFunc),
    OS3_GLPROC(glClear),
    OS3_GLPROC(glClearColor),
    OS3_GLPROC(glClearDepth),
    OS3_GLPROC(glColor3f),
    OS3_GLPROC(glColor3fv),
    OS3_GLPROC(glColor3ub),
    OS3_GLPROC(glColor3ubv),
    OS3_GLPROC(glColor4f),
    OS3_GLPROC(glColor4fv),
    OS3_GLPROC(glColor4ub),
    OS3_GLPROC(glColor4ubv),
    OS3_GLPROC(glColorMask),
    OS3_GLPROC(glColorPointer),
    OS3_GLPROC(glColorTable),
    OS3_GLPROC(glColorTableEXT),
    OS3_GLPROC(glCullFace),
    OS3_GLPROC(glDeleteTextures),
    OS3_GLPROC(glDepthFunc),
    OS3_GLPROC(glDepthMask),
    OS3_GLPROC(glDepthRange),
    OS3_GLPROC(glDisable),
    OS3_GLPROC(glDisableClientState),
    OS3_GLPROC(glDrawArrays),
    OS3_GLPROC(glDrawBuffer),
    OS3_GLPROC(glDrawElements),
    OS3_GLPROC(glEnable),
    OS3_GLPROC(glEnableClientState),
    OS3_GLPROC(glEnd),
    OS3_GLPROC(glFinish),
    OS3_GLPROC(glFlush),
    OS3_GLPROC(glFogf),
    OS3_GLPROC(glFogfv),
    OS3_GLPROC(glFogi),
    OS3_GLPROC(glFrontFace),
    OS3_GLPROC(glFrustum),
    OS3_GLPROC(glGenTextures),
    OS3_GLPROC(glGetBooleanv),
    OS3_GLPROC(glGetError),
    OS3_GLPROC(glGetFloatv),
    OS3_GLPROC(glGetIntegerv),
    OS3_GLPROC(glGetString),
    OS3_GLPROC(glHint),
    OS3_GLPROC(glIsEnabled),
    OS3_GLPROC(glLoadIdentity),
    OS3_GLPROC(glLoadMatrixd),
    OS3_GLPROC(glLoadMatrixf),
    OS3_GLPROC(glLockArrays),
    OS3_GLPROC(glMatrixMode),
    OS3_GLPROC(glMultiTexCoord2fARB),
    OS3_GLPROC(glMultiTexCoord2fvARB),
    OS3_GLPROC(glMultMatrixd),
    OS3_GLPROC(glMultMatrixf),
    OS3_GLPROC(glNormal3f),
    OS3_GLPROC(glOrtho),
    OS3_GLPROC(glPixelStorei),
    OS3_GLPROC(glPointSize),
    OS3_GLPROC(glPolygonMode),
    OS3_GLPROC(glPolygonOffset),
    OS3_GLPROC(glPopMatrix),
    OS3_GLPROC(glPushMatrix),
    OS3_GLPROC(glReadPixels),
    OS3_GLPROC(glRotated),
    OS3_GLPROC(glRotatef),
    OS3_GLPROC(glRotatefEXT),
    OS3_GLPROC(glRotatefEXTs),
    OS3_GLPROC(glScaled),
    OS3_GLPROC(glScalef),
    OS3_GLPROC(glScissor),
    OS3_GLPROC(glShadeModel),
    OS3_GLPROC(glTexCoord2f),
    OS3_GLPROC(glTexCoord2fv),
    OS3_GLPROC(glTexCoord4f),
    OS3_GLPROC(glTexCoord4fv),
    OS3_GLPROC(glTexCoordPointer),
    OS3_GLPROC(glTexEnvf),
    OS3_GLPROC(glTexEnvfv),
    OS3_GLPROC(glTexEnvi),
    OS3_GLPROC(glTexEnviv),
    OS3_GLPROC(glTexGeni),
    OS3_GLPROC(glTexImage2D),
    OS3_GLPROC(glTexParameterf),
    OS3_GLPROC(glTexParameteri),
    OS3_GLPROC(glTexSubImage2D),
    OS3_GLPROC(glTranslated),
    OS3_GLPROC(glTranslatef),
    OS3_GLPROC(glUnlockArrays),
    OS3_GLPROC(glVertex2f),
    OS3_GLPROC(glVertex2fv),
    OS3_GLPROC(glVertex3f),
    OS3_GLPROC(glVertex3fv),
    OS3_GLPROC(glVertex4f),
    OS3_GLPROC(glVertex4fv),
    OS3_GLPROC(glVertexPointer),
    OS3_GLPROC(glViewport),
    OS3_GLPROC(gluLookAt),
    OS3_GLPROC(gluPerspective),
    { NULL, NULL }
};

SDL_FunctionPointer OS3_GL_GetProcAddress(SDL_VideoDevice *_this,const char *proc)
{
    const OS3_GLProcEntry *entry;
    (void)_this;

    if(!proc){
        return NULL;
    }

    for(entry=os3_gl_procs;entry->name;entry++){
        if(SDL_strcmp(entry->name,proc)==0){
            return entry->address;
        }
    }

    /*
     * Core-name aliases for the ARB multitexture entry points MiniGL exposes.
     * This helps software which asks for the GL 1.3 spelling.
     */
    if(SDL_strcmp(proc,"glActiveTexture")==0){
        return (SDL_FunctionPointer)glActiveTextureARB;
    }
    if(SDL_strcmp(proc,"glMultiTexCoord2f")==0){
        return (SDL_FunctionPointer)glMultiTexCoord2fARB;
    }
    if(SDL_strcmp(proc,"glMultiTexCoord2fv")==0){
        return (SDL_FunctionPointer)glMultiTexCoord2fvARB;
    }

    SDL_SetError("AmigaOS3: MiniGL entry point '%s' is not available",proc);
    return NULL;
}
#endif
