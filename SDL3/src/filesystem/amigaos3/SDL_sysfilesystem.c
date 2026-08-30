#include "SDL_internal.h"
#ifdef SDL_FILESYSTEM_AMIGAOS3
#include "../SDL_sysfilesystem.h"
#include "SDL_sysfilesystem_amigaos3.h"
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

bool OS3_CreateDirTree(const char *path)
{
    char *tmp,*p;
    bool ok=true;
    if(!path || !*path) return false;
    tmp=SDL_strdup(path);
    if(!tmp) return false;
    {
        size_t n=SDL_strlen(tmp);
        while(n && tmp[n-1]=='/') tmp[--n]='\0';
    }
    p=tmp;
    while(*p && *p!=':') p++;
    if(*p==':') p++;
    for(;*p;p++){
        if(*p=='/'){
            BPTR l;
            *p='\0';
            l=Lock(tmp,ACCESS_READ);
            if(l) UnLock(l);
            else {
                l=CreateDir(tmp);
                if(l) UnLock(l);
                else if(IoErr()!=ERROR_OBJECT_EXISTS){ ok=false; *p='/'; break; }
            }
            *p='/';
        }
    }
    if(ok){
        BPTR l=Lock(tmp,ACCESS_READ);
        if(l) UnLock(l);
        else {
            l=CreateDir(tmp);
            if(l) UnLock(l);
            else if(IoErr()!=ERROR_OBJECT_EXISTS) ok=false;
        }
    }
    SDL_free(tmp);
    if(!ok) SDL_SetError("AmigaOS3: failed to create directory tree");
    return ok;
}

char *SDL_SYS_GetBasePath(void)
{
    return SDL_strdup("PROGDIR:");
}

char *SDL_SYS_GetExeName(void)
{
    char buf[256];
    if(GetProgramName(buf,sizeof(buf))) return SDL_strdup(buf);
    {
        struct Task *task=FindTask(NULL);
        if(task && task->tc_Node.ln_Name) return SDL_strdup(task->tc_Node.ln_Name);
    }
    return SDL_strdup("");
}

char *SDL_SYS_GetPrefPath(const char *org,const char *app)
{
    size_t len=SDL_strlen("ENVARC:")+4;
    char *out;
    if(org) len+=SDL_strlen(org);
    if(app) len+=SDL_strlen(app);
    out=SDL_malloc(len);
    if(!out) return NULL;
    SDL_snprintf(out,len,"ENVARC:%s%s%s%s",
        (org&&*org)?org:"",
        (org&&*org)?"/":"",
        (app&&*app)?app:"",
        (app&&*app)?"/":"");
    if(!OS3_CreateDirTree(out)){SDL_free(out);return NULL;}
    return out;
}

char *SDL_SYS_GetUserFolder(SDL_Folder folder)
{
    switch(folder){
    case SDL_FOLDER_HOME:
    case SDL_FOLDER_DOCUMENTS:
        return SDL_strdup("PROGDIR:");
    default:
        SDL_SetError("AmigaOS3: unsupported user folder");
        return NULL;
    }
}

char *SDL_SYS_GetCurrentDirectory(void)
{
    BPTR old=CurrentDir(0);
    char buf[512];
    char *out=NULL;
    if(old){
        if(NameFromLock(old,buf,sizeof(buf))) out=SDL_strdup(buf);
        CurrentDir(old);
    }
    if(!out) SDL_SetError("AmigaOS3: cannot determine current directory");
    return out;
}
#endif
