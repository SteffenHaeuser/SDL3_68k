/*
  SDL3 Mutex -- AmigaOS 3.x via Exec SignalSemaphore.
*/
#include "SDL_internal.h"

#if SDL_THREAD_AMIGAOS3

#include <SDL3/SDL_mutex.h>
#include <proto/exec.h>
#include <exec/semaphores.h>

struct SDL_Mutex {
    struct SignalSemaphore sem;
};

SDL_Mutex *SDL_CreateMutex(void)
{
    SDL_Mutex *mutex = (SDL_Mutex *)SDL_calloc(1, sizeof(*mutex));
    if (!mutex) {
        return NULL;
    }
    InitSemaphore(&mutex->sem);
    return mutex;
}

void SDL_DestroyMutex(SDL_Mutex *mutex)
{
    SDL_free(mutex);
}

void SDL_LockMutex(SDL_Mutex *mutex) SDL_NO_THREAD_SAFETY_ANALYSIS
{
    if (mutex) {
        ObtainSemaphore(&mutex->sem);
    }
}

bool SDL_TryLockMutex(SDL_Mutex *mutex)
{
    if (!mutex) {
        return SDL_SetError("Passed a NULL mutex");
    }
    return AttemptSemaphore(&mutex->sem) ? true : false;
}

void SDL_UnlockMutex(SDL_Mutex *mutex) SDL_NO_THREAD_SAFETY_ANALYSIS
{
    if (mutex) {
        ReleaseSemaphore(&mutex->sem);
    }
}

#endif /* SDL_THREAD_AMIGAOS3 */
