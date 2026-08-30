#include "SDL_internal.h"

#if SDL_VIDEO_DRIVER_AMIGAOS3

#include "SDL_os3keyboard.h"

#include "../../events/SDL_keyboard_c.h"
#include "../../events/scancodes_amiga.h"
#include <proto/keymap.h>
#include <devices/inputevent.h>

#define OS3_KEY_UP 0x80


static int OS3_Latin1ToUTF8(const unsigned char *src, int len, char *dst, int dst_size)
{
    int i, out = 0;
    if (!dst || dst_size <= 0) {
        return 0;
    }

    for (i = 0; i < len && out < dst_size - 1; ++i) {
        const unsigned int c = src[i];
        if (c < 0x80) {
            dst[out++] = (char)c;
        } else if (out + 2 < dst_size) {
            dst[out++] = (char)(0xC0 | (c >> 6));
            dst[out++] = (char)(0x80 | (c & 0x3F));
        } else {
            break;
        }
    }
    dst[out] = '\0';
    return out;
}

int OS3_TranslateRawKey(UWORD code, UWORD qualifier, APTR iaddress, char *utf8, int utf8_size)
{
    struct InputEvent ie;
    unsigned char local[16];
    LONG result;

    if (!KeymapBase || (code & OS3_KEY_UP)) {
        return 0;
    }

    SDL_zero(ie);
    ie.ie_Class = IECLASS_RAWKEY;
    ie.ie_Code = code & ~OS3_KEY_UP;
    ie.ie_Qualifier = qualifier;
    ie.ie_EventAddress = iaddress;

    result = MapRawKey(&ie, (STRPTR)local, sizeof(local), NULL);
    if (result <= 0) {
        return 0;
    }
    if (result > (LONG)sizeof(local)) {
        result = sizeof(local);
    }

    return OS3_Latin1ToUTF8(local, (int)result, utf8, utf8_size);
}

bool OS3_StartTextInput(SDL_VideoDevice *_this, SDL_Window *window, SDL_PropertiesID props)
{
    (void)_this; (void)window; (void)props;
    return true;
}

bool OS3_StopTextInput(SDL_VideoDevice *_this, SDL_Window *window)
{
    (void)_this; (void)window;
    return true;
}

void OS3_InitKeyboard(SDL_VideoDevice *_this)
{
    (void)_this;
    SDL_SetScancodeName(SDL_SCANCODE_LGUI, "Left Amiga");
    SDL_SetScancodeName(SDL_SCANCODE_RGUI, "Right Amiga");
    SDL_SetScancodeName(SDL_SCANCODE_LCTRL, "Control");
}

void OS3_QuitKeyboard(SDL_VideoDevice *_this)
{
    (void)_this;
}

void OS3_HandleRawKey(SDL_VideoDevice *_this, SDL_Window *window,
                      UWORD code, UWORD qualifier)
{
    const UWORD raw = code & ~OS3_KEY_UP;
    const bool down = (code & OS3_KEY_UP) ? false : true;
    SDL_Scancode scancode;

    (void)_this;
    (void)qualifier;

    if (raw >= SDL_arraysize(amiga_scancode_table)) {
        return;
    }

    scancode = amiga_scancode_table[raw];
    if (scancode == SDL_SCANCODE_UNKNOWN) {
        return;
    }

    if (window) {
        SDL_SetKeyboardFocus(window);
    }

    SDL_SendKeyboardKey(0, SDL_GLOBAL_KEYBOARD_ID, (int)raw, scancode, down);
}

#endif
