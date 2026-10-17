/*
  SDL3 Counting Semaphore -- AmigaOS 3.x.
  Infinite waits use Exec signals. Finite waits use the SDL timer.
*/
#include "SDL_internal.h"

#if SDL_THREAD_AMIGAOS3

#include <SDL3/SDL_mutex.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/tasks.h>
#include <exec/semaphores.h>
#include <devices/timer.h>

typedef struct OS3_SemWaiter {
    struct OS3_SemWaiter *next;
    struct Task *task;
    BYTE signal_bit;
    bool queued;
} OS3_SemWaiter;

struct SDL_Semaphore {
    struct SignalSemaphore lock;
    Uint32 count;
    OS3_SemWaiter *head;
    OS3_SemWaiter *tail;
};

SDL_Semaphore *SDL_CreateSemaphore(Uint32 initial_value)
{
    SDL_Semaphore *sem = (SDL_Semaphore *)SDL_calloc(1, sizeof(*sem));
    if (!sem) {
        return NULL;
    }

    InitSemaphore(&sem->lock);
    sem->count = initial_value;
    return sem;
}

void SDL_DestroySemaphore(SDL_Semaphore *sem)
{
    if (sem) {
        SDL_free(sem);
    }
}

static bool OS3_RemoveWaiter(SDL_Semaphore *sem, OS3_SemWaiter *target)
{
    OS3_SemWaiter *prev = NULL;
    OS3_SemWaiter *cur;
    bool found = false;

    ObtainSemaphore(&sem->lock);
    for (cur = sem->head; cur; cur = cur->next) {
        if (cur == target) {
            if (prev) {
                prev->next = cur->next;
            } else {
                sem->head = cur->next;
            }
            if (sem->tail == cur) {
                sem->tail = prev;
            }
            cur->queued = false;
            found = true;
            break;
        }
        prev = cur;
    }
    ReleaseSemaphore(&sem->lock);
    return found;
}

/* Every timed wait used to create a message port and an I/O request and
 * open timer.device, then close it all again: hundreds of times a second in
 * a game (SDL's timer thread on every tick, delays, condition waits), with
 * 64-bit divisions on a CPU without them.  Each thread keeps one open
 * request now, in its TLS, closed when the thread ends. */
typedef struct OS3_TimerCache {
    struct MsgPort *port;
    struct timerequest *tr;
} OS3_TimerCache;
static SDL_TLSID OS3_timer_tls;

static void SDLCALL OS3_FreeTimerCache(void *value)
{
    OS3_TimerCache *cache = (OS3_TimerCache *)value;
    if (!cache) {
        return;
    }
    CloseDevice((struct IORequest *)cache->tr);
    DeleteIORequest((struct IORequest *)cache->tr);
    DeleteMsgPort(cache->port);
    SDL_free(cache);
}

static OS3_TimerCache *OS3_GetTimerCache(void)
{
    OS3_TimerCache *cache = (OS3_TimerCache *)SDL_GetTLS(&OS3_timer_tls);
    if (cache) {
        return cache;
    }
    cache = (OS3_TimerCache *)SDL_calloc(1, sizeof(*cache));
    if (!cache) {
        return NULL;
    }
    cache->port = CreateMsgPort();
    cache->tr = cache->port ? (struct timerequest *)CreateIORequest(cache->port, sizeof(*cache->tr)) : NULL;
    if (!cache->tr || OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *)cache->tr, 0) != 0) {
        if (cache->tr) {
            DeleteIORequest((struct IORequest *)cache->tr);
        }
        if (cache->port) {
            DeleteMsgPort(cache->port);
        }
        SDL_free(cache);
        SDL_SetError("timer.device for a timed wait failed");
        return NULL;
    }
    SDL_SetTLS(&OS3_timer_tls, cache, OS3_FreeTimerCache);
    return cache;
}

static bool OS3_TimerWait(ULONG sem_mask, Sint64 timeoutNS, bool *sem_signalled)
{
    OS3_TimerCache *cache = OS3_GetTimerCache();
    struct timerequest *tr;
    ULONG port_mask;
    ULONG got;

    *sem_signalled = false;
    if (!cache) {
        return false;
    }
    tr = cache->tr;
    port_mask = 1UL << cache->port->mp_SigBit;

    /* Up to 4.29 s (nearly every wait) in 32-bit arithmetic. */
    if (timeoutNS <= 0xffffffffLL) {
        const ULONG ns = (ULONG)timeoutNS;
        const ULONG usec = ns / 1000U + (ns % 1000U ? 1U : 0U);
        tr->tr_time.tv_secs = usec / 1000000U;
        tr->tr_time.tv_micro = usec % 1000000U;
    } else {
        const Uint64 usec = ((Uint64)timeoutNS + 999ULL) / 1000ULL;
        tr->tr_time.tv_secs = (ULONG)(usec / 1000000ULL);
        tr->tr_time.tv_micro = (ULONG)(usec % 1000000ULL);
    }
    tr->tr_node.io_Command = TR_ADDREQUEST;
    SetSignal(0, port_mask);  /* a reply from an earlier wait left the bit set */
    SendIO((struct IORequest *)tr);

    got = Wait(sem_mask | port_mask);

    if (got & sem_mask) {
        *sem_signalled = true;
    }
    if (!CheckIO((struct IORequest *)tr)) {
        AbortIO((struct IORequest *)tr);
    }
    WaitIO((struct IORequest *)tr);
    return true;
}

static bool OS3_TryTakeSemaphore(SDL_Semaphore *sem)
{
    bool result = false;

    ObtainSemaphore(&sem->lock);
    if (sem->count) {
        --sem->count;
        result = true;
    }
    ReleaseSemaphore(&sem->lock);
    return result;
}

bool SDL_WaitSemaphoreTimeoutNS(SDL_Semaphore *sem, Sint64 timeoutNS)
{
    OS3_SemWaiter waiter;
    BYTE sig;
    bool woke_by_sem = false;

    if (!sem) {
        return SDL_SetError("Passed a NULL semaphore");
    }

    if (OS3_TryTakeSemaphore(sem)) {
        return true;
    }

    if (timeoutNS == 0) {
        return false;
    }

    sig = AllocSignal(-1);
    if (sig == -1) {
        return SDL_SetError("No Exec signal bit available");
    }

    waiter.next = NULL;
    waiter.task = FindTask(NULL);
    waiter.signal_bit = sig;
    waiter.queued = false;

    ObtainSemaphore(&sem->lock);

    /* Recheck while holding the queue lock to close the post/wait race. */
    if (sem->count) {
        --sem->count;
        ReleaseSemaphore(&sem->lock);
        FreeSignal(sig);
        return true;
    }

    waiter.queued = true;
    if (sem->tail) {
        sem->tail->next = &waiter;
    } else {
        sem->head = &waiter;
    }
    sem->tail = &waiter;
    ReleaseSemaphore(&sem->lock);

    if (timeoutNS < 0) {
        Wait(1UL << sig);
        woke_by_sem = true;
    } else {
        if (!OS3_TimerWait(1UL << sig, timeoutNS, &woke_by_sem)) {
            /* Timer setup failed: remove ourselves if still queued. */
            OS3_RemoveWaiter(sem, &waiter);
            FreeSignal(sig);
            return false;
        }
    }

    if (!woke_by_sem) {
        /*
         * Timer fired. If the waiter is still queued, timeout wins. If it
         * was already removed by SDL_SignalSemaphore(), the semaphore wins
         * even if both signals arrived at nearly the same time.
         */
        if (OS3_RemoveWaiter(sem, &waiter)) {
            FreeSignal(sig);
            return false;
        }
        woke_by_sem = true;
    }

    FreeSignal(sig);
    return woke_by_sem;
}

Uint32 SDL_GetSemaphoreValue(SDL_Semaphore *sem)
{
    Uint32 value;

    if (!sem) {
        SDL_SetError("Passed a NULL semaphore");
        return 0;
    }

    ObtainSemaphore(&sem->lock);
    value = sem->count;
    ReleaseSemaphore(&sem->lock);
    return value;
}

void SDL_SignalSemaphore(SDL_Semaphore *sem)
{
    OS3_SemWaiter *waiter = NULL;

    if (!sem) {
        return;
    }

    ObtainSemaphore(&sem->lock);

    if (sem->head) {
        waiter = sem->head;
        sem->head = waiter->next;
        waiter->queued = false;
        if (!sem->head) {
            sem->tail = NULL;
        }
    } else {
        ++sem->count;
    }

    ReleaseSemaphore(&sem->lock);

    if (waiter) {
        Signal(waiter->task, 1UL << waiter->signal_bit);
    }
}

#endif /* SDL_THREAD_AMIGAOS3 */
