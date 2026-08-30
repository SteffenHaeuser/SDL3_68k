#include "SDL_internal.h"
#if SDL_VIDEO_DRIVER_AMIGAOS3
#include "SDL_os3messagebox.h"
#include "SDL_os3window.h"
#include "SDL_os3video.h"
#include <proto/exec.h>
#include <proto/intuition.h>
#define BUTTON_BUF_SIZE 1024

static char *OS3_Buttons(const SDL_MessageBoxData *d)
{
    int i; char *s=SDL_calloc(1,BUTTON_BUF_SIZE);
    if(!s) return NULL;
    for(i=0;i<d->numbuttons;i++){
        SDL_strlcat(s,d->buttons[i].text,BUTTON_BUF_SIZE);
        if(i+1<d->numbuttons) SDL_strlcat(s,"|",BUTTON_BUF_SIZE);
    }
    return s;
}
bool OS3_ShowMessageBox(const SDL_MessageBoxData *d,int *buttonid)
{
    struct IntuitionBase *oldbase=IntuitionBase;
    struct Library *temp=NULL;
    struct Window *win=NULL;
    char *buttons;
    struct EasyStruct es;
    LONG chosen;
    if(!IntuitionBase){
        temp=OpenLibrary("intuition.library",37);
        IntuitionBase=(struct IntuitionBase *)temp;
        if(!temp) return SDL_SetError("AmigaOS3: cannot open intuition.library");
    }
    if(d->window && d->window->internal) win=((SDL_WindowData *)d->window->internal)->syswin;
    buttons=OS3_Buttons(d);
    if(!buttons){if(temp){CloseLibrary(temp);IntuitionBase=oldbase;}return false;}
    SDL_zero(es); es.es_StructSize=sizeof(es); es.es_Title=(STRPTR)d->title;
    es.es_TextFormat=(STRPTR)d->message; es.es_GadgetFormat=buttons;
    chosen=EasyRequestArgs(win,&es,NULL,NULL);
    if(buttonid && d->numbuttons>0){
        if(chosen==0) *buttonid=d->buttons[d->numbuttons-1].buttonID;
        else if(chosen>0 && chosen<=d->numbuttons) *buttonid=d->buttons[chosen-1].buttonID;
    }
    SDL_free(buttons);
    if(temp){CloseLibrary(temp);IntuitionBase=oldbase;}
    return true;
}
#endif
