#include "SDL_internal.h"

#if SDL_VIDEO_DRIVER_AMIGAOS3

#include "SDL_os3events.h"
#include "SDL_os3keyboard.h"
#include "SDL_os3mouse.h"

#include "../../events/SDL_keyboard_c.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_windowevents_c.h"

void OS3_PumpEvents(SDL_VideoDevice *_this)
{
    SDL_Window *sw;

    for (sw = _this->windows; sw; sw = sw->next) {
        SDL_WindowData *d = sw->internal;
        struct IntuiMessage *m;

        if (!d || !d->syswin || !d->syswin->UserPort) {
            continue;
        }

        while ((m = (struct IntuiMessage *)GetMsg(d->syswin->UserPort))) {
            ULONG cls = m->Class;
            UWORD code = m->Code;
            UWORD qualifier = m->Qualifier;
            WORD mx = m->MouseX;
            WORD my = m->MouseY;
            APTR iaddress = m->IAddress;
            char text_utf8[32];
            int text_len = 0;
            int inner_w = d->syswin->GZZWidth;
            int inner_h = d->syswin->GZZHeight;

            /* MapRawKey needs IAddress for dead-key handling, so translate
               before replying the IntuiMessage, then send SDL events after. */
            if (cls == IDCMP_RAWKEY && !(code & 0x80) &&
                SDL_TextInputActive(sw) && !OS3_HandleRawMouseWheel(sw, code)) {
                text_len = OS3_TranslateRawKey(code, qualifier, iaddress,
                                               text_utf8, sizeof(text_utf8));
            }

            ReplyMsg((struct Message *)m);

            switch (cls) {
            case IDCMP_RAWKEY:
                if (!OS3_HandleRawMouseWheel(sw, code)) {
                    OS3_HandleRawKey(_this, sw, code, qualifier);
                    if (text_len > 0) {
                        SDL_SendKeyboardText(text_utf8);
                    }
                }
                break;

            case IDCMP_MOUSEBUTTONS:
                OS3_HandleMouseButton(sw, code);
                break;

            case IDCMP_MOUSEMOVE:
                if (SDL_GetRelativeMouseMode()) {
                    OS3_HandleRelativeMouseMotion(sw, mx, my);
                } else {
                    OS3_HandleMouseMotion(sw,
                        mx - d->syswin->BorderLeft,
                        my - d->syswin->BorderTop);
                }
                break;

            case IDCMP_CLOSEWINDOW:
                SDL_SendWindowEvent(sw, SDL_EVENT_WINDOW_CLOSE_REQUESTED, 0, 0);
                break;

            case IDCMP_NEWSIZE:
                SDL_SendWindowEvent(sw, SDL_EVENT_WINDOW_RESIZED, inner_w, inner_h);
                break;

            case IDCMP_CHANGEWINDOW:
                SDL_SendWindowEvent(sw, SDL_EVENT_WINDOW_MOVED,
                                    d->syswin->LeftEdge, d->syswin->TopEdge);
                break;

            case IDCMP_ACTIVEWINDOW:
                SDL_SetKeyboardFocus(sw);
                SDL_SetMouseFocus(sw);
                SDL_SendWindowEvent(sw, SDL_EVENT_WINDOW_FOCUS_GAINED, 0, 0);
                break;

            case IDCMP_INACTIVEWINDOW:
                if (SDL_GetKeyboardFocus() == sw) {
                    SDL_SetKeyboardFocus(NULL);
                }
                if (SDL_GetMouseFocus() == sw) {
                    SDL_SetMouseFocus(NULL);
                }
                SDL_SendWindowEvent(sw, SDL_EVENT_WINDOW_FOCUS_LOST, 0, 0);
                break;

            default:
                break;
            }
        }
    }
}

#endif
