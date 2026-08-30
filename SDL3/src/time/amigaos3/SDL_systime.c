#include "SDL_internal.h"

#ifdef SDL_TIME_AMIGAOS3

#include "../SDL_time_c.h"
#include "../../locale/amigaos3/SDL_syslocale_amigaos3.h"

#include <proto/exec.h>
#include <proto/timer.h>
#include <devices/timer.h>

/* Implemented by src/timer/amigaos3/SDL_systimer.c. */
extern bool OS3_GetSystemTime1978(Uint64 *seconds, Uint32 *microseconds);

#define OS3_UNIX_TO_AMIGA_EPOCH 252460800LL

static bool OS3_IsLeap(int y)
{
    return ((y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0));
}

static void OS3_DaysToCivil(Sint64 days, int *year, int *month, int *day)
{
    /* Howard Hinnant's civil-from-days algorithm, epoch 1970-01-01. */
    Sint64 z = days + 719468;
    Sint64 era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
    int y = (int)yoe + (int)era * 400;
    unsigned doy = doe - (365*yoe + yoe/4 - yoe/100);
    unsigned mp = (5*doy + 2) / 153;
    unsigned d = doy - (153*mp + 2)/5 + 1;
    unsigned m = mp + (mp < 10 ? 3 : -9);
    y += (m <= 2);

    *year = y; *month = (int)m; *day = (int)d;
}

void SDL_GetSystemTimeLocalePreferences(SDL_DateFormat *df, SDL_TimeFormat *tf)
{
    OS3_GetLocaleDateTimePreferences(df, tf);
}

bool SDL_GetCurrentTime(SDL_Time *ticks)
{
    Uint64 amiga_seconds;
    Uint32 micros;
    Sint64 unix_local;
    Sint64 unix_utc;
    int offset;

    CHECK_PARAM(!ticks) {
        return SDL_InvalidParamError("ticks");
    }

    if (!OS3_GetSystemTime1978(&amiga_seconds, &micros)) {
        return SDL_SetError("AmigaOS3: unable to read timer.device system time");
    }

    /*
     * Classic AmigaOS system time is conventionally local time since
     * 1978-01-01. Convert it to UTC before exposing it as SDL_Time.
     */
    unix_local = (Sint64)amiga_seconds + OS3_UNIX_TO_AMIGA_EPOCH;
    offset = OS3_GetLocaleUTCOffsetSeconds();
    unix_utc = unix_local - offset;

    *ticks = SDL_SECONDS_TO_NS(unix_utc) + SDL_US_TO_NS(micros);
    return true;
}

bool SDL_TimeToDateTime(SDL_Time ticks, SDL_DateTime *dt, bool localTime)
{
    Sint64 sec;
    Sint64 days;
    Sint64 sod;
    int offset = 0;
    int dow;

    CHECK_PARAM(!dt) {
        return SDL_InvalidParamError("dt");
    }

    sec = SDL_NS_TO_SECONDS(ticks);
    if (localTime) {
        offset = OS3_GetLocaleUTCOffsetSeconds();
        sec += offset;
    }

    days = sec / SDL_SECONDS_PER_DAY;
    sod = sec % SDL_SECONDS_PER_DAY;
    if (sod < 0) {
        sod += SDL_SECONDS_PER_DAY;
        --days;
    }

    SDL_zero(*dt);
    OS3_DaysToCivil(days, &dt->year, &dt->month, &dt->day);
    dt->hour = (int)(sod / 3600);
    dt->minute = (int)((sod / 60) % 60);
    dt->second = (int)(sod % 60);
    dt->nanosecond = (int)(ticks % SDL_NS_PER_SECOND);
    if (dt->nanosecond < 0) dt->nanosecond += SDL_NS_PER_SECOND;

    dow = (int)((days + 4) % 7); /* 1970-01-01 was Thursday. */
    if (dow < 0) dow += 7;
    dt->day_of_week = dow;
    dt->utc_offset = localTime ? offset : 0;

    return true;
}

#endif
