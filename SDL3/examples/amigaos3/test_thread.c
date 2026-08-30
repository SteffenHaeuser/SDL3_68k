#include <SDL3/SDL.h>
#include <proto/dos.h>

typedef struct TestData {
    SDL_Mutex *mutex;
    SDL_Semaphore *ready;
    int counter;
    SDL_TLSID tls;
} TestData;

static int SDLCALL worker(void *arg)
{
    TestData *data = (TestData *)arg;

    PutStr("thread: worker entered\n");

    if (!SDL_SetTLS(&data->tls, (const void *)0x1234, NULL)) {
        PutStr("thread: SDL_SetTLS failed\n");
        return 10;
    }
    if (SDL_GetTLS(&data->tls) != (void *)0x1234) {
        PutStr("thread: SDL_GetTLS mismatch\n");
        return 11;
    }

    SDL_LockMutex(data->mutex);
    data->counter += 1;
    SDL_UnlockMutex(data->mutex);

    SDL_SignalSemaphore(data->ready);
    PutStr("thread: worker leaving\n");
    return 42;
}

int main(void)
{
    TestData data;
    SDL_Thread *thread;
    int status = -1;

    PutStr("thread: entered main\n");

    SDL_zero(data);
    data.mutex = SDL_CreateMutex();
    data.ready = SDL_CreateSemaphore(0);

    if (!data.mutex || !data.ready) {
        PutStr("thread: sync creation failed\n");
        return 1;
    }

    PutStr("thread: creating worker\n");
    thread = SDL_CreateThread(worker, "amigaos3-test", &data);
    if (!thread) {
        PutStr("thread: SDL_CreateThread failed\n");
        return 2;
    }

    PutStr("thread: waiting semaphore\n");
    SDL_WaitSemaphore(data.ready);
    PutStr("thread: semaphore signalled\n");

    PutStr("thread: joining worker\n");
    SDL_WaitThread(thread, &status);

    if (data.counter != 1 || status != 42) {
        PutStr("thread: RESULT FAIL\n");
        return 4;
    }

    SDL_DestroySemaphore(data.ready);
    SDL_DestroyMutex(data.mutex);

    PutStr("thread: RESULT OK\n");
    return 0;
}
