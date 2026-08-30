/*
  SDL3 Threading -- AmigaOS 3.x via DOS Processes / Exec Tasks.
*/
#include "SDL_internal.h"

#if SDL_THREAD_AMIGAOS3

#include <SDL3/SDL_thread.h>
#include "../SDL_thread_c.h"
#include "../SDL_systhread.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/tasks.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>

typedef struct OS3_ThreadData {
    SDL_Thread *thread;
    struct Task *task;
    volatile bool complete;
    volatile bool detached;
    struct Task *join_task;
    BYTE join_signal;
} OS3_ThreadData;

static void OS3_ThreadEntry(void)
{
    struct Task *me = FindTask(NULL);
    OS3_ThreadData *td = (OS3_ThreadData *)me->tc_UserData;
    bool detached;

    if (!td || !td->thread) {
        return;
    }

    SDL_RunThread(td->thread);

    Forbid();
    td->complete = true;
    detached = td->detached;
    if (td->join_task && td->join_signal >= 0) {
        Signal(td->join_task, 1UL << td->join_signal);
    }
    Permit();

    if (detached) {
        SDL_free(td);
    }
}

bool SDL_SYS_CreateThread(SDL_Thread *thread,
                          SDL_FunctionPointer pfnBeginThread,
                          SDL_FunctionPointer pfnEndThread)
{
    OS3_ThreadData *td;
    struct Process *proc;
    char name[80];

    (void)pfnBeginThread;
    (void)pfnEndThread;

    td = (OS3_ThreadData *)SDL_calloc(1, sizeof(*td));
    if (!td) {
        return false;
    }

    td->thread = thread;
    td->join_signal = -1;

    SDL_snprintf(name, sizeof(name), "SDL %s",
                 thread->name ? thread->name : "thread");

    /*
     * Prevent the new Process from running until tc_UserData is installed.
     * AmigaOS3 CreateNewProcTags() has no NP_UserData tag.
     */
    Forbid();

    proc = CreateNewProcTags(
        NP_Entry, (ULONG)OS3_ThreadEntry,
        NP_Name, (ULONG)name,
        NP_StackSize, (ULONG)(thread->stacksize ? thread->stacksize : 32768),
        NP_Priority, 0,
        NP_Input, 0,
        NP_Output, 0,
        NP_CloseInput, FALSE,
        NP_CloseOutput, FALSE,
        TAG_DONE);

    if (!proc) {
        Permit();
        SDL_free(td);
        return SDL_SetError("CreateNewProcTags() failed");
    }

    td->task = &proc->pr_Task;
    proc->pr_Task.tc_UserData = td;

    thread->handle = td;
    thread->threadid = (SDL_ThreadID)(Uint32)&proc->pr_Task;

    Permit();
    return true;
}

void SDL_SYS_SetupThread(const char *name)
{
    (void)name;
}

SDL_ThreadID SDL_GetCurrentThreadID(void)
{
    return (SDL_ThreadID)(Uint32)FindTask(NULL);
}

bool SDL_SYS_SetThreadPriority(SDL_ThreadPriority priority)
{
    BYTE value = 0;

    switch (priority) {
    case SDL_THREAD_PRIORITY_LOW:
        value = -5;
        break;
    case SDL_THREAD_PRIORITY_HIGH:
        value = 5;
        break;
    case SDL_THREAD_PRIORITY_TIME_CRITICAL:
        value = 10;
        break;
    case SDL_THREAD_PRIORITY_NORMAL:
    default:
        value = 0;
        break;
    }

    SetTaskPri(FindTask(NULL), value);
    return true;
}

void SDL_SYS_WaitThread(SDL_Thread *thread)
{
    OS3_ThreadData *td = thread ? thread->handle : NULL;

    if (!td) {
        return;
    }

    if (!td->complete) {
        BYTE sig = AllocSignal(-1);
        if (sig == -1) {
            /* Very rare fallback if this task has no free signal bit. */
            while (!td->complete) {
                Wait(SIGBREAKF_CTRL_C);
            }
        } else {
            Forbid();
            if (!td->complete) {
                td->join_task = FindTask(NULL);
                td->join_signal = sig;
                Permit();
                Wait(1UL << sig);
            } else {
                Permit();
            }

            Forbid();
            td->join_task = NULL;
            td->join_signal = -1;
            Permit();
            FreeSignal(sig);
        }
    }

    thread->handle = NULL;
    SDL_free(td);
}

void SDL_SYS_DetachThread(SDL_Thread *thread)
{
    OS3_ThreadData *td = thread ? thread->handle : NULL;
    bool complete;

    if (!td) {
        return;
    }

    Forbid();
    complete = td->complete;
    if (!complete) {
        td->detached = true;
    }
    Permit();

    thread->handle = NULL;

    if (complete) {
        SDL_free(td);
    }
}

#endif /* SDL_THREAD_AMIGAOS3 */
