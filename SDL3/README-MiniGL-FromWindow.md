# SDL3 AmigaOS3: MiniGL FromWindow

The AmigaOS3 backend now creates its native Intuition window before creating
a GL context. `mglCreateContextFromWindow()` borrows that window; destroying
the context leaves it intact. A new context can then be created on the same
SDL window. SDL destroys any remaining context before closing its own window.

## Wrap a window opened by the application

SDL3 uses properties rather than SDL2's SDL_CreateWindowFrom API:

```c
/* SDL_Init(SDL_INIT_VIDEO) must have succeeded. */
SDL_PropertiesID props = SDL_CreateProperties();
SDL_SetPointerProperty(props,
    SDL_PROP_WINDOW_CREATE_AMIGAOS3_WINDOW_POINTER, native_window);
SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
SDL_Window *window = SDL_CreateWindowWithProperties(props);
SDL_DestroyProperties(props);
/* Check window, then create a context and check its result: */
SDL_GLContext context = SDL_GL_CreateContext(window);
/* Render and call SDL_GL_SwapWindow(window). */
/* When finished: */
SDL_GL_DestroyContext(context);
SDL_DestroyWindow(window);
/* native_window and its screen still belong to the application. */
```

The new property is declared in `include/SDL3/SDL_video.h`; its string is
`SDL.window.create.amigaos3.window`. It accepts a live `struct Window *`.
Do not use `SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN` for this
case: the native window is external, but SDL still creates its MiniGL context.

Keep the native window and screen alive until both context and SDL wrapper
have been destroyed. One wrapper and one context per native window are
supported. The current external-window path requires OpenGL and true-color
RTG with CyberGraphX-compatible services. Null pointers, duplicate wrappers,
and non-OpenGL wrapping are rejected. SDL cannot validate arbitrary pointer
lifetimes.

The application keeps its IDCMP loop. SDL does not consume/reply to that
window's messages, change IDCMP subscriptions, replace its pointer, change its
title, move/resize/raise it, or close it. Translate host input with SDL_PushEvent
if your application needs SDL events. SDL_PollEvent alone will not collect
host input. The SDL wrapper is marked SDL_WINDOW_EXTERNAL.

After a host resize, call SDL_GetWindowSizeInPixels before drawing and update
your GL viewport/projection. This also synchronizes MiniGL's drawable without
changing the native window, preserving the previously current context.
Swap checks the drawable size too. SDL's logical size starts with the host
client size; use pixel size to render after native host resizes.

## Fullscreen and ownership

Initial fullscreen uses SDL's existing native screen/window path before the
GL context is created. The old MiniGL-owned fullscreen screen and BGRA32-only
mode precheck have been removed; context creation uses the actual drawable.
For an SDL-owned window, destroy the GL context before a fullscreen transition
and create a new context afterwards. Transitions that would replace a native
window with a live context, and transitions for external windows, are rejected.
Border/resizable changes that require native-window recreation are ignored
while a GL context is attached. Context migration and sharing are not added.

## SDK and build

The previously supplied include_v27_2 MiniGL headers are bundled in `include/`,
which is already on the Makefile include path. A runtime guard checks dispatch
ABI, table size, current-context pointer and the FromWindow entry. Use a
matching `minigl.library` and SDK `libminigl.a`. The MiniGL implementation and
rebuilt binaries are not supplied.

The private window structure changed: perform a clean rebuild.

```
make -f Makefile.amigaos3 clean
make -f Makefile.amigaos3 both
make -f Makefile.amigaos3 gl-existing-clib2
make -f Makefile.amigaos3 gl-existing-libnix
```

The new example is `examples/amigaos3/test_gl_existing_window.c`. It wraps a
host window, destroys/recreates its context, renders a blue client area,
handles native close/resize messages, and accesses/closes the host window
after destroying the SDL wrapper. Ordinary examples remain available.
The software-only build does not gain MiniGL support from this change.
Swap-interval handling remains unchanged (0/1 recorded, backend presentation).

## Validation status

SDK header copies, archive integrity, and applying the diff to the original
ZIP contents are checked during packaging. No compilation or PiStorm runtime
test has passed in this environment; the installed Amiga cross-compiler could
not start because of a signal-pipe creation error (Win32 error 5).

On target, also test normal SDL-created GL windows, initial fullscreen,
multiple windows/current-context switching, native resize, failed context
creation followed by retry, and SDL_DestroyWindow with an attached context.
Check that older/incomplete MiniGL dispatch tables fail without calling the
FromWindow slot. Host screens/windows must survive SDL wrapper destruction.
