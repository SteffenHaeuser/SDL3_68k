#include "SDL_internal.h"
#include "../SDL_syslocale.h"
#include "SDL_syslocale_amigaos3.h"

#include <proto/exec.h>
#include <proto/locale.h>
#include <libraries/locale.h>

struct LocaleBase *LocaleBase = NULL;

static const char *OS3_LanguageToISO(const char *name)
{
    if (!name || !*name) return NULL;

    if (!SDL_strcasecmp(name, "deutsch") || !SDL_strcasecmp(name, "german")) return "de";
    if (!SDL_strcasecmp(name, "english")) return "en";
    if (!SDL_strcasecmp(name, "français") || !SDL_strcasecmp(name, "francais") || !SDL_strcasecmp(name, "french")) return "fr";
    if (!SDL_strcasecmp(name, "italiano") || !SDL_strcasecmp(name, "italian")) return "it";
    if (!SDL_strcasecmp(name, "español") || !SDL_strcasecmp(name, "espanol") || !SDL_strcasecmp(name, "spanish")) return "es";
    if (!SDL_strcasecmp(name, "português") || !SDL_strcasecmp(name, "portugues") || !SDL_strcasecmp(name, "portuguese")) return "pt";
    if (!SDL_strcasecmp(name, "nederlands") || !SDL_strcasecmp(name, "dutch")) return "nl";
    if (!SDL_strcasecmp(name, "dansk") || !SDL_strcasecmp(name, "danish")) return "da";
    if (!SDL_strcasecmp(name, "svenska") || !SDL_strcasecmp(name, "swedish")) return "sv";
    if (!SDL_strcasecmp(name, "norsk") || !SDL_strcasecmp(name, "norwegian")) return "no";
    if (!SDL_strcasecmp(name, "suomi") || !SDL_strcasecmp(name, "finnish")) return "fi";
    if (!SDL_strcasecmp(name, "polski") || !SDL_strcasecmp(name, "polish")) return "pl";
    if (!SDL_strcasecmp(name, "čeština") || !SDL_strcasecmp(name, "czech")) return "cs";
    if (!SDL_strcasecmp(name, "magyar") || !SDL_strcasecmp(name, "hungarian")) return "hu";
    if (!SDL_strcasecmp(name, "ελληνικά") || !SDL_strcasecmp(name, "greek")) return "el";

    return NULL;
}

static struct Locale *OS3_OpenCurrentLocale(struct LocaleBase **opened_base)
{
    struct LocaleBase *base;
    struct Locale *loc;

    *opened_base = NULL;
    base = (struct LocaleBase *)OpenLibrary("locale.library", 38);
    if (!base) return NULL;

    LocaleBase = base;
    loc = OpenLocale(NULL);
    if (!loc) {
        CloseLibrary((struct Library *)base);
        LocaleBase = NULL;
        return NULL;
    }

    *opened_base = base;
    return loc;
}

static void OS3_CloseCurrentLocale(struct Locale *loc, struct LocaleBase *base)
{
    if (loc) CloseLocale(loc);
    if (base) CloseLibrary((struct Library *)base);
    LocaleBase = NULL;
}

bool SDL_SYS_GetPreferredLocales(char *buf, size_t buflen)
{
    struct LocaleBase *base;
    struct Locale *loc;
    int i;
    bool any = false;

    if (!buf || buflen == 0) return false;
    buf[0] = '\0';

    loc = OS3_OpenCurrentLocale(&base);
    if (loc) {
        for (i = 0; i < 10 && loc->loc_PrefLanguages[i]; ++i) {
            const char *iso = OS3_LanguageToISO(loc->loc_PrefLanguages[i]);
            if (iso) {
                if (any) SDL_strlcat(buf, ",", buflen);
                SDL_strlcat(buf, iso, buflen);
                any = true;
            }
        }
        OS3_CloseCurrentLocale(loc, base);
    }

    if (!any) SDL_strlcpy(buf, "en", buflen);
    return true;
}

int OS3_GetLocaleUTCOffsetSeconds(void)
{
    struct LocaleBase *base;
    struct Locale *loc;
    int result = 0;

    loc = OS3_OpenCurrentLocale(&base);
    if (loc) {
        /*
         * Amiga loc_GMTOffset is minutes from GMT:
         *   positive = west, negative = east.
         * SDL DateTime wants local - UTC in seconds, so invert it.
         */
        result = -(int)loc->loc_GMTOffset * 60;
        OS3_CloseCurrentLocale(loc, base);
    }
    return result;
}

void OS3_GetLocaleDateTimePreferences(SDL_DateFormat *df, SDL_TimeFormat *tf)
{
    struct LocaleBase *base;
    struct Locale *loc;

    loc = OS3_OpenCurrentLocale(&base);
    if (!loc) return;

    if (df && loc->loc_ShortDateFormat) {
        const char *s = loc->loc_ShortDateFormat;
        const char *d = SDL_strstr(s, "%d");
        const char *m = SDL_strstr(s, "%m");
        const char *y = SDL_strstr(s, "%y");
        const char *Y = SDL_strstr(s, "%Y");
        const char *yp = Y ? Y : y;

        if (yp && d && m) {
            if (yp < d && yp < m) *df = SDL_DATE_FORMAT_YYYYMMDD;
            else if (d < m) *df = SDL_DATE_FORMAT_DDMMYYYY;
            else *df = SDL_DATE_FORMAT_MMDDYYYY;
        }
    }

    if (tf && loc->loc_ShortTimeFormat) {
        const char *s = loc->loc_ShortTimeFormat;
        if (SDL_strstr(s, "%I") || SDL_strstr(s, "%l")) *tf = SDL_TIME_FORMAT_12HR;
        else if (SDL_strstr(s, "%H") || SDL_strstr(s, "%k")) *tf = SDL_TIME_FORMAT_24HR;
    }

    OS3_CloseCurrentLocale(loc, base);
}
