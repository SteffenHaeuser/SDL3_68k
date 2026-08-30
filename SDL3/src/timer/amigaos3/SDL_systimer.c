/*
  Simple DirectMedia Layer
  AmigaOS 3.x timer backend for SDL3.

  Uses timer.device ReadEClock() for the high-resolution monotonic counter
  and dos.library Delay() for sleeps.  This deliberately avoids Unix
  select()/nanosleep() dependencies.
*/
#include "SDL_internal.h"

#if defined(SDL_TIMER_AMIGAOS3) || defined(SDL_TIMERS_DISABLED)

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <devices/timer.h>

/* Required by proto/timer.h inline calls on classic AmigaOS. */
struct Device *TimerBase = NULL;

static struct MsgPort *timer_port = NULL;
static struct timerequest *timer_io = NULL;
static ULONG eclock_freq = 0;

static bool OS3_InitEClock(void)
{
    if (TimerBase && eclock_freq) {
        return true;
    }

    timer_port = CreateMsgPort();
    if (!timer_port) {
        return false;
    }

    timer_io = (struct timerequest *)CreateIORequest(timer_port, sizeof(struct timerequest));
    if (!timer_io) {
        DeleteMsgPort(timer_port);
        timer_port = NULL;
        return false;
    }

    if (OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_ECLOCK,
                   (struct IORequest *)timer_io, 0) != 0) {
        DeleteIORequest((struct IORequest *)timer_io);
        timer_io = NULL;
        DeleteMsgPort(timer_port);
        timer_port = NULL;
        return false;
    }

    TimerBase = (struct Device *)timer_io->tr_node.io_Device;

    {
        struct EClockVal now;
        eclock_freq = ReadEClock(&now);
    }

    return eclock_freq != 0;
}

static Uint64 OS3_EClockToU64(const struct EClockVal *value)
{
    return ((Uint64)value->ev_hi << 32) | (Uint64)value->ev_lo;
}


bool OS3_GetSystemTime1978(Uint64 *seconds, Uint32 *microseconds)
{
    struct timeval tv;

    if (!seconds || !microseconds) {
        return false;
    }
    if (!OS3_InitEClock()) {
        return false;
    }

    GetSysTime(&tv);
    *seconds = (Uint64)(Uint32)tv.tv_secs;
    *microseconds = (Uint32)tv.tv_micro;
    return true;
}

Uint64 SDL_GetPerformanceCounter(void)
{
    struct EClockVal now;

    if (!OS3_InitEClock()) {
        return 0;
    }

    ReadEClock(&now);
    return OS3_EClockToU64(&now);
}

Uint64 SDL_GetPerformanceFrequency(void)
{
    if (!OS3_InitEClock()) {
        return 1000;
    }
    return (Uint64)eclock_freq;
}

void SDL_SYS_DelayNS(Uint64 ns)
{
    Uint64 ms;
    ULONG ticks;

    if (ns == 0) {
        Delay(0);
        return;
    }

    /* dos.library Delay() is VBlank-tick based.  Round upward so the
       requested delay is never shortened.  Classic PAL uses 50 Hz;
       this deliberately favours portability over sub-tick precision. */
    ms = (ns + 999999ULL) / 1000000ULL;
    ticks = (ULONG)((ms + 19ULL) / 20ULL);
    if (ticks == 0) {
        ticks = 1;
    }
    Delay(ticks);
}

#endif /* SDL_TIMER_AMIGAOS3 || SDL_TIMERS_DISABLED */
