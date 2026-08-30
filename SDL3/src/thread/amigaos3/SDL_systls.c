/*
  SDL3 TLS -- AmigaOS 3.x, keyed by Exec Task.
*/
#include "SDL_internal.h"

#if SDL_THREAD_AMIGAOS3

#include "../SDL_thread_c.h"

#include <proto/exec.h>
#include <exec/tasks.h>
#include <exec/semaphores.h>

typedef struct OS3_TLSEntry {
    struct OS3_TLSEntry *next;
    struct Task *task;
    SDL_TLSData *data;
} OS3_TLSEntry;

static struct SignalSemaphore tls_lock;
static OS3_TLSEntry *tls_list;
static bool tls_initialized;

void SDL_SYS_InitTLSData(void)
{
    if (!tls_initialized) {
        InitSemaphore(&tls_lock);
        tls_list = NULL;
        tls_initialized = true;
    }
}

static OS3_TLSEntry *OS3_FindTLSEntry(struct Task *task)
{
    OS3_TLSEntry *entry;
    for (entry = tls_list; entry; entry = entry->next) {
        if (entry->task == task) {
            return entry;
        }
    }
    return NULL;
}

SDL_TLSData *SDL_SYS_GetTLSData(void)
{
    OS3_TLSEntry *entry;
    SDL_TLSData *data = NULL;

    if (!tls_initialized) {
        SDL_SYS_InitTLSData();
    }

    ObtainSemaphore(&tls_lock);
    entry = OS3_FindTLSEntry(FindTask(NULL));
    if (entry) {
        data = entry->data;
    }
    ReleaseSemaphore(&tls_lock);
    return data;
}

bool SDL_SYS_SetTLSData(SDL_TLSData *data)
{
    struct Task *task = FindTask(NULL);
    OS3_TLSEntry *entry;

    if (!tls_initialized) {
        SDL_SYS_InitTLSData();
    }

    ObtainSemaphore(&tls_lock);
    entry = OS3_FindTLSEntry(task);

    if (entry) {
        entry->data = data;
    } else {
        entry = (OS3_TLSEntry *)SDL_calloc(1, sizeof(*entry));
        if (!entry) {
            ReleaseSemaphore(&tls_lock);
            return false;
        }
        entry->task = task;
        entry->data = data;
        entry->next = tls_list;
        tls_list = entry;
    }

    ReleaseSemaphore(&tls_lock);
    return true;
}

void SDL_SYS_QuitTLSData(void)
{
    OS3_TLSEntry *entry;

    if (!tls_initialized) {
        return;
    }

    ObtainSemaphore(&tls_lock);
    entry = tls_list;
    tls_list = NULL;
    ReleaseSemaphore(&tls_lock);

    while (entry) {
        OS3_TLSEntry *next = entry->next;
        SDL_free(entry);
        entry = next;
    }

    tls_initialized = false;
}

#endif /* SDL_THREAD_AMIGAOS3 */
