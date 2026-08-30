#include "SDL_internal.h"
#include "../SDL_dialog.h"
#include "../../video/amigaos3/SDL_os3window.h"
#include <proto/exec.h>
#include <proto/asl.h>
#include <libraries/asl.h>

struct Library *AslBase = NULL;

typedef struct {
    char *title,*accept,*cancel,*location,*pattern;
    struct Window *window;
    bool many,save,dir_only;
    SDL_DialogFileCallback callback;
    void *userdata;
} OS3_DialogArgs;

static void OS3_Free(OS3_DialogArgs *a){SDL_free(a->title);SDL_free(a->accept);SDL_free(a->cancel);SDL_free(a->location);SDL_free(a->pattern);SDL_free(a);}

static char *OS3_BuildASLPattern(const SDL_DialogFileFilter *filters,int nfilters)
{
    size_t need=8;
    int i;
    char *out,*p;
    bool any=false;

    if(!filters || nfilters<=0) return NULL;

    for(i=0;i<nfilters;i++){
        const char *s=filters[i].pattern;
        if(!s || !*s) continue;
        if(!SDL_strcmp(s,"*")) return SDL_strdup("#?");
        need+=SDL_strlen(s)*4+16;
        any=true;
    }
    if(!any) return NULL;

    out=SDL_malloc(need);
    if(!out) return NULL;
    p=out;
    *p++='#'; *p++='?'; *p++='.'; *p++='(';

    {
        bool first=true;
        for(i=0;i<nfilters;i++){
            const char *s=filters[i].pattern;
            if(!s || !*s || !SDL_strcmp(s,"*")) continue;
            while(*s){
                const char *e=SDL_strchr(s,';');
                size_t n=e?(size_t)(e-s):SDL_strlen(s);
                if(n){
                    if(!first) *p++='|';
                    SDL_memcpy(p,s,n);
                    p+=n;
                    first=false;
                }
                if(!e) break;
                s=e+1;
            }
        }
    }

    *p++=')'; *p='\0';
    return out;
}

static char *OS3_Join(const char *drawer,const char *file)
{
    size_t dl=SDL_strlen(drawer),fl=SDL_strlen(file),n=dl+fl+2; char *p=SDL_malloc(n);
    if(!p)return NULL;
    SDL_snprintf(p,n,"%s%s%s",drawer,(dl&&(drawer[dl-1]==':'||drawer[dl-1]=='/'))?"":"/",file);
    return p;
}
static int OS3_DialogThread(void *ptr)
{
    OS3_DialogArgs *a=ptr;
    struct FileRequester *r;
    struct Library *lib=OpenLibrary("asl.library",38);
    AslBase=lib;
    if(!lib){a->callback(a->userdata,NULL,-1);OS3_Free(a);return 0;}
    r=(struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_Window,a->window,
        ASLFR_TitleText,a->title,
        ASLFR_PositiveText,a->accept,
        ASLFR_NegativeText,a->cancel,
        ASLFR_InitialDrawer,a->location,
        ASLFR_InitialPattern,a->pattern,
        ASLFR_DoPatterns,a->pattern?TRUE:FALSE,
        ASLFR_DoMultiSelect,a->many,
        ASLFR_DoSaveMode,a->save,
        ASLFR_DrawersOnly,a->dir_only,
        ASLFR_PrivateIDCMP,TRUE,
        TAG_DONE);
    if(!r){a->callback(a->userdata,NULL,-1);}
    else if(AslRequestTags(r,TAG_DONE)){
        if(a->dir_only){
            const char *paths[2]={r->fr_Drawer,NULL}; a->callback(a->userdata,paths,-1);
        } else if(r->fr_NumArgs>0){
            int i; char **paths=SDL_calloc((size_t)r->fr_NumArgs+1,sizeof(char*));
            if(!paths) a->callback(a->userdata,NULL,-1);
            else {
                for(i=0;i<r->fr_NumArgs;i++) paths[i]=OS3_Join(r->fr_Drawer,r->fr_ArgList[i].wa_Name);
                a->callback(a->userdata,(const char * const *)paths,-1);
                for(i=0;i<r->fr_NumArgs;i++) SDL_free(paths[i]);
                SDL_free(paths);
            }
        } else {
            char *p=OS3_Join(r->fr_Drawer,r->fr_File);
            if(p){const char *paths[2]={p,NULL};a->callback(a->userdata,paths,-1);SDL_free(p);}
            else a->callback(a->userdata,NULL,-1);
        }
    } else {
        const char *paths[1]={NULL}; a->callback(a->userdata,paths,-1);
    }
    if(r) FreeAslRequest(r);
    CloseLibrary(lib); AslBase=NULL; OS3_Free(a); return 0;
}
void SDL_SYS_ShowFileDialogWithProperties(SDL_FileDialogType type,SDL_DialogFileCallback cb,void *ud,SDL_PropertiesID props)
{
    OS3_DialogArgs *a=SDL_calloc(1,sizeof(*a)); SDL_Window *w; SDL_Thread *th;
    if(!a){cb(ud,NULL,-1);return;}
    w=SDL_GetPointerProperty(props,SDL_PROP_FILE_DIALOG_WINDOW_POINTER,NULL);
    if(w && w->internal) a->window=((SDL_WindowData *)w->internal)->syswin;
    a->location=SDL_strdup(SDL_GetStringProperty(props,SDL_PROP_FILE_DIALOG_LOCATION_STRING,""));
    a->title=SDL_strdup(SDL_GetStringProperty(props,SDL_PROP_FILE_DIALOG_TITLE_STRING,
        type==SDL_FILEDIALOG_SAVEFILE?"Save file...":type==SDL_FILEDIALOG_OPENFOLDER?"Open folder...":"Open file..."));
    a->accept=SDL_strdup(SDL_GetStringProperty(props,SDL_PROP_FILE_DIALOG_ACCEPT_STRING,"Ok"));
    a->cancel=SDL_strdup(SDL_GetStringProperty(props,SDL_PROP_FILE_DIALOG_CANCEL_STRING,"Cancel"));
    a->many=SDL_GetBooleanProperty(props,SDL_PROP_FILE_DIALOG_MANY_BOOLEAN,false);
    a->save=(type==SDL_FILEDIALOG_SAVEFILE); a->dir_only=(type==SDL_FILEDIALOG_OPENFOLDER);
    if(!a->dir_only){
        const SDL_DialogFileFilter *filters=(const SDL_DialogFileFilter *)SDL_GetPointerProperty(props,SDL_PROP_FILE_DIALOG_FILTERS_POINTER,NULL);
        int nfilters=(int)SDL_GetNumberProperty(props,SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER,0);
        a->pattern=OS3_BuildASLPattern(filters,nfilters);
    }
    a->callback=cb;a->userdata=ud;
    th=SDL_CreateThread(OS3_DialogThread,"SDL file dialog",a);
    if(!th){cb(ud,NULL,-1);OS3_Free(a);return;}
    SDL_DetachThread(th);
}
