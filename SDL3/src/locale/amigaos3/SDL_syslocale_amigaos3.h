#ifndef SDL_syslocale_amigaos3_h_
#define SDL_syslocale_amigaos3_h_

#include "SDL_internal.h"

extern int OS3_GetLocaleUTCOffsetSeconds(void);
extern void OS3_GetLocaleDateTimePreferences(SDL_DateFormat *df, SDL_TimeFormat *tf);

#endif
