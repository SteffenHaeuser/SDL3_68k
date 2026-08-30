#include <SDL3/SDL.h>
#include <stdio.h>

int main(void)
{
    SDL_Time now;
    SDL_DateTime utc, local;
    SDL_DateFormat df;
    SDL_TimeFormat tf;
    SDL_Locale **locs;
    int count=0,i;

    if(!SDL_GetCurrentTime(&now)){printf("time failed: %s\n",SDL_GetError());return 1;}
    SDL_TimeToDateTime(now,&utc,false);
    SDL_TimeToDateTime(now,&local,true);
    SDL_GetDateTimeLocalePreferences(&df,&tf);

    printf("UTC   : %04d-%02d-%02d %02d:%02d:%02d offset=%d\n",
        utc.year,utc.month,utc.day,utc.hour,utc.minute,utc.second,utc.utc_offset);
    printf("LOCAL : %04d-%02d-%02d %02d:%02d:%02d offset=%d\n",
        local.year,local.month,local.day,local.hour,local.minute,local.second,local.utc_offset);
    printf("formats: date=%d time=%d\n",(int)df,(int)tf);

    locs=SDL_GetPreferredLocales(&count);
    printf("preferred locales: %d\n",count);
    for(i=0;i<count;i++) printf("  %s%s%s\n",locs[i]->language,locs[i]->country?"_":"",locs[i]->country?locs[i]->country:"");
    SDL_free(locs);
    return 0;
}
