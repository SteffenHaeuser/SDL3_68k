#include "SDL_internal.h"

#ifdef SDL_JOYSTICK_AMIGAOS3

#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/lowlevel.h>
#include <libraries/lowlevel.h>

struct Library *LowLevelBase = NULL;

typedef struct joystick_hwdata {
    int port;
    ULONG type;
    ULONG last_state;
} joystick_hwdata;

typedef struct {
    int port;
    ULONG type;
    SDL_JoystickID instance_id;
} OS3_JoyDevice;

static OS3_JoyDevice devices[4];
static int num_devices;

static void OS3_ScanDevices(void)
{
    int port;

    num_devices=0;
    if (!LowLevelBase) {
        return;
    }

    for (port=0;port<4;port++) {
        ULONG state=ReadJoyPort((ULONG)port);
        ULONG type;

        if (state==JP_TYPE_NOTAVAIL) {
            continue;
        }

        type=state&JP_TYPE_MASK;

        /*
         * Match the proven Quake2 lowlevel.library behaviour:
         * anything that exists and is not a mouse is a potential joystick.
         *
         * Some adapters/emulators do not immediately report JP_TYPE_JOYSTK
         * even though direction/button bits are usable. Preserve a detected
         * CD32/game-controller type; otherwise fall back to ordinary joystick.
         */
        if (type==JP_TYPE_MOUSE) {
            continue;
        }

        if (type!=JP_TYPE_GAMECTLR) {
            type=JP_TYPE_JOYSTK;
        }

        devices[num_devices].port=port;
        devices[num_devices].type=type;
        devices[num_devices].instance_id=(SDL_JoystickID)(port+1);
        num_devices++;
    }
}

static bool OS3_JoystickInit(void)
{
    int port;

    LowLevelBase=OpenLibrary("lowlevel.library",40);
    if (!LowLevelBase) {
        return SDL_SetError("AmigaOS3: cannot open lowlevel.library V40+");
    }

    /*
     * Put all four lowlevel ports into autosense mode. Ports 2/3 can be
     * provided by four-player/splitter adapters, so do not assume only the
     * two motherboard control ports exist.
     */
    for (port=0;port<4;port++) {
        SetJoyPortAttrs((ULONG)port,
                        SJA_Type,SJA_TYPE_AUTOSENSE,
                        TAG_DONE);
    }

    OS3_ScanDevices();
    return true;
}

static int OS3_JoystickGetCount(void) { return num_devices; }
static void OS3_JoystickDetect(void)
{
    /* Ports are fixed, but emulators/USB stacks can change what lowlevel
       reports at runtime. Keep the discovery snapshot fresh. */
    OS3_ScanDevices();
}

static bool OS3_JoystickIsDevicePresent(Uint16 vendor_id,Uint16 product_id,Uint16 version,const char *name)
{
    (void)vendor_id;(void)product_id;(void)version;(void)name;
    return false;
}

static const char *OS3_JoystickGetDeviceName(int idx)
{
    if (idx<0 || idx>=num_devices) return NULL;
    return devices[idx].type==JP_TYPE_GAMECTLR ? "Amiga lowlevel game controller" : "Amiga lowlevel joystick";
}
static const char *OS3_JoystickGetDevicePath(int idx)
{
    static const char *paths[4]={
        "lowlevel:port0",
        "lowlevel:port1",
        "lowlevel:port2",
        "lowlevel:port3"
    };
    int port;

    if (idx<0 || idx>=num_devices) {
        return NULL;
    }
    port=devices[idx].port;
    return (port>=0 && port<4) ? paths[port] : NULL;
}
static int OS3_JoystickGetSteamSlot(int idx){(void)idx;return -1;}
static int OS3_JoystickGetPlayerIndex(int idx){return (idx>=0&&idx<num_devices)?devices[idx].port:-1;}
static void OS3_JoystickSetPlayerIndex(int idx,int player){(void)idx;(void)player;}

static SDL_GUID OS3_JoystickGetDeviceGUID(int idx)
{
    SDL_GUID guid;
    SDL_zero(guid);
    if (idx>=0 && idx<num_devices) {
        const char *tag=devices[idx].type==JP_TYPE_GAMECTLR ? "AMIGA-LOWLEVEL-CD32" : "AMIGA-LOWLEVEL-JOY";
        SDL_memcpy(guid.data,tag,SDL_min((size_t)16,SDL_strlen(tag)));
        guid.data[15]=(Uint8)devices[idx].port;
    }
    return guid;
}
static SDL_JoystickID OS3_JoystickGetDeviceInstanceID(int idx)
{
    return (idx>=0&&idx<num_devices)?devices[idx].instance_id:0;
}

static bool OS3_JoystickOpen(SDL_Joystick *joy,int idx)
{
    joystick_hwdata *hw;
    if (idx<0 || idx>=num_devices) return SDL_SetError("Invalid Amiga joystick index");
    hw=(joystick_hwdata *)SDL_calloc(1,sizeof(*hw));
    if (!hw) return false;
    hw->port=devices[idx].port;
    hw->type=devices[idx].type;
    hw->last_state=ReadJoyPort(hw->port);
    joy->hwdata=hw;
    joy->naxes=2;
    joy->nhats=0;
    joy->nbuttons=(hw->type==JP_TYPE_GAMECTLR)?7:2;
    return true;
}

static bool OS3_UnsupportedRumble(SDL_Joystick *j,Uint16 a,Uint16 b){(void)j;(void)a;(void)b;return SDL_Unsupported();}
static bool OS3_UnsupportedLED(SDL_Joystick *j,Uint8 r,Uint8 g,Uint8 b){(void)j;(void)r;(void)g;(void)b;return SDL_Unsupported();}
static bool OS3_UnsupportedEffect(SDL_Joystick *j,const void *p,int n){(void)j;(void)p;(void)n;return SDL_Unsupported();}
static bool OS3_UnsupportedSensors(SDL_Joystick *j,bool e){(void)j;(void)e;return SDL_Unsupported();}

static void OS3_JoystickUpdate(SDL_Joystick *joy)
{
    joystick_hwdata *hw=joy->hwdata;
    ULONG s;
    Sint16 x=0,y=0;
    static const ULONG buttons[7]={
        JPF_BUTTON_RED,JPF_BUTTON_BLUE,JPF_BUTTON_YELLOW,JPF_BUTTON_GREEN,
        JPF_BUTTON_FORWARD,JPF_BUTTON_REVERSE,JPF_BUTTON_PLAY
    };
    int i;
    Uint64 now=SDL_GetTicksNS();
    if (!hw) return;
    s=ReadJoyPort(hw->port);

    if (s&JPF_JOY_LEFT) x=SDL_MIN_SINT16;
    else if (s&JPF_JOY_RIGHT) x=SDL_MAX_SINT16;
    if (s&JPF_JOY_UP) y=SDL_MIN_SINT16;
    else if (s&JPF_JOY_DOWN) y=SDL_MAX_SINT16;

    SDL_SendJoystickAxis(now,joy,0,x);
    SDL_SendJoystickAxis(now,joy,1,y);

    for (i=0;i<joy->nbuttons;i++) {
        SDL_SendJoystickButton(now,joy,(Uint8)i,(s&buttons[i])?true:false);
    }
    hw->last_state=s;
}

static void OS3_JoystickClose(SDL_Joystick *joy)
{
    SDL_free(joy->hwdata);
    joy->hwdata=NULL;
}
static void OS3_JoystickQuit(void)
{
    int port;

    num_devices=0;
    if (LowLevelBase) {
        for (port=0;port<4;port++) {
            SetJoyPortAttrs((ULONG)port,
                            SJA_Type,SJA_TYPE_AUTOSENSE,
                            TAG_DONE);
        }
        CloseLibrary(LowLevelBase);
        LowLevelBase=NULL;
    }
}
static bool OS3_JoystickGetGamepadMapping(int idx,SDL_GamepadMapping *out)
{
    (void)idx;(void)out;
    return false;
}

SDL_JoystickDriver SDL_AMIGAOS3_JoystickDriver={
    OS3_JoystickInit,
    OS3_JoystickGetCount,
    OS3_JoystickDetect,
    OS3_JoystickIsDevicePresent,
    OS3_JoystickGetDeviceName,
    OS3_JoystickGetDevicePath,
    OS3_JoystickGetSteamSlot,
    OS3_JoystickGetPlayerIndex,
    OS3_JoystickSetPlayerIndex,
    OS3_JoystickGetDeviceGUID,
    OS3_JoystickGetDeviceInstanceID,
    OS3_JoystickOpen,
    OS3_UnsupportedRumble,
    OS3_UnsupportedRumble,
    OS3_UnsupportedLED,
    OS3_UnsupportedEffect,
    OS3_UnsupportedSensors,
    OS3_JoystickUpdate,
    OS3_JoystickClose,
    OS3_JoystickQuit,
    OS3_JoystickGetGamepadMapping
};

#endif
