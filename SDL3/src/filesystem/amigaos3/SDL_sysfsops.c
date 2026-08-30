#include "SDL_internal.h"
#ifdef SDL_FSOPS_AMIGAOS3
#include "../SDL_sysfilesystem.h"
#include "SDL_sysfilesystem_amigaos3.h"
#include "../../locale/amigaos3/SDL_syslocale_amigaos3.h"
#include <proto/dos.h>
#include <dos/dos.h>

#define OS3_UNIX_TO_AMIGA_EPOCH 252460800LL
#define OS3_TICKS_PER_SECOND 50

static SDL_Time OS3_DateStampToSDLTime(const struct DateStamp *ds)
{
    Sint64 local_seconds;
    Sint64 utc_seconds;
    Sint64 ns;
    int offset;

    /*
     * AmigaDOS DateStamp is local time since 1978-01-01.
     * SDL_Time is UTC nanoseconds since 1970-01-01.
     */
    local_seconds = ((Sint64)ds->ds_Days * 86400LL) +
                    ((Sint64)ds->ds_Minute * 60LL) +
                    ((Sint64)ds->ds_Tick / OS3_TICKS_PER_SECOND) +
                    OS3_UNIX_TO_AMIGA_EPOCH;
    offset = OS3_GetLocaleUTCOffsetSeconds();
    utc_seconds = local_seconds - offset;
    ns = SDL_SECONDS_TO_NS(utc_seconds);
    ns += ((Sint64)(ds->ds_Tick % OS3_TICKS_PER_SECOND) * SDL_NS_PER_SECOND) /
          OS3_TICKS_PER_SECOND;
    return (SDL_Time)ns;
}


bool SDL_SYS_EnumerateDirectory(const char *path,SDL_EnumerateDirectoryCallback cb,void *userdata)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    bool ok=true;
    char dirname[1024];
    size_t len;
    if(!path||!*path||!cb) return false;
    lock=Lock((STRPTR)path,ACCESS_READ);
    if(!lock) return SDL_SetError("AmigaOS3: cannot lock directory");
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if(!fib){UnLock(lock);return SDL_OutOfMemory();}
    len=SDL_strlen(path);
    SDL_snprintf(dirname,sizeof(dirname),"%s%s",path,(len && (path[len-1]==':'||path[len-1]=='/'))?"":"/");
    if(Examine(lock,fib)){
        while(ExNext(lock,fib)){
            SDL_EnumerationResult r;
            if(!SDL_strcmp(fib->fib_FileName,".")||!SDL_strcmp(fib->fib_FileName,"..")) continue;
            r=cb(userdata,dirname,fib->fib_FileName);
            if(r==SDL_ENUM_SUCCESS) break;
            if(r==SDL_ENUM_FAILURE){ok=false;SDL_SetError("Directory callback failed");break;}
        }
        if(ok && IoErr()!=ERROR_NO_MORE_ENTRIES) { ok=false; SDL_SetError("AmigaOS3: directory enumeration failed"); }
    } else {ok=false;SDL_SetError("AmigaOS3: Examine failed");}
    FreeDosObject(DOS_FIB,fib);
    UnLock(lock);
    return ok;
}

bool SDL_SYS_RemovePath(const char *path)
{
    if(DeleteFile((STRPTR)path)) return true;
    if(IoErr()==ERROR_OBJECT_NOT_FOUND) return true;
    return SDL_SetError("AmigaOS3: DeleteFile failed");
}
bool SDL_SYS_RenamePath(const char *a,const char *b)
{
    if(Rename((STRPTR)a,(STRPTR)b)) return true;
    return SDL_SetError("AmigaOS3: Rename failed");
}
bool SDL_SYS_CopyFile(const char *a,const char *b)
{
    BPTR in=0,out=0;
    char buf[32768];
    LONG n;
    bool ok=false;
    in=Open((STRPTR)a,MODE_OLDFILE); if(!in) goto done;
    out=Open((STRPTR)b,MODE_NEWFILE); if(!out) goto done;
    while((n=Read(in,buf,sizeof(buf)))>0){
        if(Write(out,buf,n)!=n) goto done;
    }
    ok=(n==0);
done:
    if(out) Close(out);
    if(in) Close(in);
    if(!ok) SDL_SetError("AmigaOS3: file copy failed");
    return ok;
}
bool SDL_SYS_CreateDirectory(const char *path){return OS3_CreateDirTree(path);}
bool SDL_SYS_GetPathInfo(const char *path,SDL_PathInfo *info)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    SDL_zero(*info);
    lock=Lock((STRPTR)path,ACCESS_READ);
    if(!lock){info->type=SDL_PATHTYPE_NONE;return SDL_SetError("AmigaOS3: path not found");}
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if(!fib){UnLock(lock);return SDL_OutOfMemory();}
    if(!Examine(lock,fib)){FreeDosObject(DOS_FIB,fib);UnLock(lock);return SDL_SetError("AmigaOS3: Examine failed");}
    if(fib->fib_DirEntryType>0) info->type=SDL_PATHTYPE_DIRECTORY;
    else {info->type=SDL_PATHTYPE_FILE;info->size=(Uint64)(Uint32)fib->fib_Size;}

    /*
     * Classic AmigaDOS FileInfoBlock exposes one DateStamp: the last
     * modification time. There is no portable creation/access timestamp
     * available here, so leave those at zero.
     */
    info->modify_time=OS3_DateStampToSDLTime(&fib->fib_Date);
    FreeDosObject(DOS_FIB,fib); UnLock(lock); return true;
}
#endif
