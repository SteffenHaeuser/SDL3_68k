/*
  SDL3 AmigaOS3 AHI backend.
  Playback-only first implementation.
*/
#include "SDL_internal.h"

#ifdef SDL_AUDIO_DRIVER_AMIGAOS3

#include "../SDL_audio_c.h"
#include "../SDL_sysaudio.h"
#include "SDL_os3audio.h"

#include <proto/exec.h>
#include <exec/memory.h>
#include <devices/ahi.h>

static bool OS3_OpenAHI(OS3AudioData *data)
{
    if (data->device_open) {
        return true;
    }

    data->reply_port = CreateMsgPort();
    if (!data->reply_port) {
        SDL_SetError("AmigaOS3 AHI: CreateMsgPort failed");
        return false;
    }

    data->request[0] = (struct AHIRequest *)CreateIORequest(
        data->reply_port, sizeof(struct AHIRequest));
    if (!data->request[0]) {
        DeleteMsgPort(data->reply_port);
        data->reply_port = NULL;
        SDL_SetError("AmigaOS3 AHI: CreateIORequest failed");
        return false;
    }

    data->request[0]->ahir_Version = 4;

    /* Match the proven SDL1/SDL2 OS3 backend exactly: unit 0, flags NULL. */
    if (OpenDevice((CONST_STRPTR)AHINAME, 0,
                   (struct IORequest *)data->request[0], NULL) != 0) {
        DeleteIORequest((struct IORequest *)data->request[0]);
        DeleteMsgPort(data->reply_port);
        data->request[0] = NULL;
        data->reply_port = NULL;
        SDL_SetError("AmigaOS3 AHI: OpenDevice failed");
        return false;
    }

    /*
     * IMPORTANT: this mirrors the working SDL1/SDL2 backend.
     * req[1] is a plain MEMF_PUBLIC clone of the opened request, not a second
     * CreateIORequest().
     */
    data->request[1] = (struct AHIRequest *)AllocVec(
        sizeof(struct AHIRequest), MEMF_PUBLIC);
    if (!data->request[1]) {
        CloseDevice((struct IORequest *)data->request[0]);
        DeleteIORequest((struct IORequest *)data->request[0]);
        DeleteMsgPort(data->reply_port);
        data->request[0] = NULL;
        data->reply_port = NULL;
        SDL_OutOfMemory();
        return false;
    }

    SDL_memcpy(data->request[1], data->request[0], sizeof(struct AHIRequest));

    data->device_open = true;
    data->current = 0;
    data->playing = 0;
    data->in_flight[0] = false;
    data->in_flight[1] = false;
    data->link = NULL;
    data->last_submitted = -1;
    return true;
}

static void OS3_FinishRequest(OS3AudioData *data, int slot, bool abort_it)
{
    struct AHIRequest *req;

    if (!data || slot < 0 || slot > 1 || !data->in_flight[slot]) {
        return;
    }

    req = data->request[slot];
    if (!req) {
        data->in_flight[slot] = false;
        return;
    }

    /*
     * Never AbortIO/WaitIO a request that was never submitted.
     * If it is still active, optionally abort it.  WaitIO is then called
     * exactly once to collect the completed request/reply message.
     */
    if (!CheckIO((struct IORequest *)req) && abort_it) {
        AbortIO((struct IORequest *)req);
    }
    WaitIO((struct IORequest *)req);
    data->in_flight[slot] = false;
}

static void OS3_CloseAHI(OS3AudioData *data)
{
    if (!data) return;

    if (data->device_open) {
        /*
         * Break/collect only requests that were actually submitted.
         * This also makes the AHI availability probe safe: playing==0 means
         * there is no IO to abort or wait for.
         */
        OS3_FinishRequest(data, 1, true);
        OS3_FinishRequest(data, 0, true);

        if (data->request[0]) {
            CloseDevice((struct IORequest *)data->request[0]);
        }
    }

    if (data->request[1]) {
        FreeVec(data->request[1]);
        data->request[1] = NULL;
    }

    if (data->request[0]) {
        DeleteIORequest((struct IORequest *)data->request[0]);
        data->request[0] = NULL;
    }

    if (data->reply_port) {
        DeleteMsgPort(data->reply_port);
        data->reply_port = NULL;
    }

    data->playing = 0;
    data->current = 0;
    data->in_flight[0] = false;
    data->in_flight[1] = false;
    data->link = NULL;
    data->last_submitted = -1;
    data->device_open = false;
}

static bool OS3_AudioAvailable(void)
{
    OS3AudioData data;
    bool ok;

    SDL_zero(data);
    ok = OS3_OpenAHI(&data);
    OS3_CloseAHI(&data);
    return ok;
}

static void OS3_DetectDevices(SDL_AudioDevice **default_output,
                              SDL_AudioDevice **default_recording)
{
    SDL_AudioSpec output;
    (void)default_recording;

    SDL_zero(output);
    output.freq = 44100;
    output.format = SDL_AUDIO_S16BE;
    output.channels = 2;

    *default_output = SDL_AddAudioDevice(false,
                                         "AHI default output",
                                         &output,
                                         SDL_strdup("default"));
    *default_recording = NULL;
}

static bool OS3_OpenDevice(SDL_AudioDevice *device)
{
    OS3AudioData *data;
    int frame_size;

    if (device->recording) {
        return SDL_SetError("AmigaOS3 AHI: recording is not implemented");
    }

    data = (OS3AudioData *)SDL_calloc(1, sizeof(*data));
    if (!data) {
        return false;
    }
    device->hidden = data;

    /*
     * Native AHI playback formats used by the proven classic SDL1/SDL2
     * backends. SDL3 converts application formats that do not map directly.
     */
    if (SDL_AUDIO_BITSIZE(device->spec.format) <= 8) {
        device->spec.format = SDL_AUDIO_S8;
    } else {
        device->spec.format = SDL_AUDIO_S16BE;
    }

    if (device->spec.channels <= 1) {
        device->spec.channels = 1;
    } else {
        device->spec.channels = 2;
    }

    if (device->spec.freq <= 0) {
        device->spec.freq = 44100;
    }

    if (device->spec.format == SDL_AUDIO_S8) {
        data->ahi_type = (device->spec.channels == 1) ? AHIST_M8S : AHIST_S8S;
    } else {
        data->ahi_type = (device->spec.channels == 1) ? AHIST_M16S : AHIST_S16S;
    }

    device->sample_frames = SDL_GetDefaultSampleFramesFromFreq(device->spec.freq);
    SDL_UpdatedAudioDeviceFormat(device);

    frame_size = SDL_AUDIO_FRAMESIZE(device->spec);
    if (frame_size <= 0) {
        SDL_free(data);
        device->hidden = NULL;
        return SDL_SetError("AmigaOS3 AHI: invalid audio frame size");
    }

    /*
     * SDL_UpdatedAudioDeviceFormat() computes buffer_size from sample_frames,
     * but enforce a useful minimum to keep AHI request overhead reasonable.
     */
    data->buffer_size = device->buffer_size;
    if (data->buffer_size < 2048) {
        data->buffer_size = 2048;
        data->buffer_size -= data->buffer_size % frame_size;
        device->sample_frames = data->buffer_size / frame_size;
        SDL_UpdatedAudioDeviceFormat(device);
        data->buffer_size = device->buffer_size;
    }

    /*
     * AHI device buffers must use memory suitable for device access.
     * The working SDL1/SDL2 AmigaOS3 backends use AllocVec(MEMF_PUBLIC).
     */
    data->buffer[0] = (Uint8 *)AllocVec(data->buffer_size, MEMF_PUBLIC);
    data->buffer[1] = (Uint8 *)AllocVec(data->buffer_size, MEMF_PUBLIC);
    if (!data->buffer[0] || !data->buffer[1]) {
        if (data->buffer[0]) FreeVec(data->buffer[0]);
        if (data->buffer[1]) FreeVec(data->buffer[1]);
        SDL_free(data);
        device->hidden = NULL;
        return SDL_OutOfMemory();
    }

    SDL_memset(data->buffer[0], device->silence_value, data->buffer_size);
    SDL_memset(data->buffer[1], device->silence_value, data->buffer_size);

    return true;
}

static void OS3_ThreadInit(SDL_AudioDevice *device)
{
    OS3AudioData *data = (OS3AudioData *)device->hidden;

    /*
     * The Exec reply-port signal belongs to this audio task, therefore AHI
     * is opened here, not in OpenDevice().
     */
    if (!OS3_OpenAHI(data)) {
        data->thread_open_failed = true;
        SDL_AudioDeviceDisconnected(device);
    }
}

static void OS3_ThreadDeinit(SDL_AudioDevice *device)
{
    OS3AudioData *data = (OS3AudioData *)device->hidden;
    OS3_CloseAHI(data);
}

static bool OS3_WaitDevice(SDL_AudioDevice *device)
{
    OS3AudioData *data = (OS3AudioData *)device->hidden;
    const int next_slot = data->current;

    if (data->thread_open_failed || !data->device_open) {
        return false;
    }

    /*
     * True double buffering:
     *
     * After PlayDevice(), current already points at the buffer SDL will fill
     * on the NEXT iteration.  If that buffer is still being played by AHI,
     * wait for it here.  The other buffer remains queued/playing, so SDL can
     * refill this one without inserting a gap.
     *
     * First iteration: req0 sent, current=1, req1 unused -> return immediately.
     * Second: req1 linked to req0, current=0 -> wait for req0.
     * Third: refill/send req0 linked to req1, current=1 -> wait for req1.
     */
    if (data->in_flight[next_slot]) {
        WaitIO((struct IORequest *)data->request[next_slot]);
        data->in_flight[next_slot] = false;
    }

    return true;
}

static Uint8 *OS3_GetDeviceBuf(SDL_AudioDevice *device, int *buffer_size)
{
    OS3AudioData *data = (OS3AudioData *)device->hidden;
    if (buffer_size) {
        *buffer_size = data->buffer_size;
    }
    return data->buffer[data->current];
}



static bool OS3_PlayDevice(SDL_AudioDevice *device,
                           const Uint8 *buffer, int buflen)
{
    OS3AudioData *data = (OS3AudioData *)device->hidden;
    const int slot = data->current;
    const int previous_slot = slot ^ 1;
    struct AHIRequest *req = data->request[slot];

    if (!data->device_open || data->thread_open_failed) {
        return false;
    }

    if (data->in_flight[slot]) {
        return SDL_SetError("AmigaOS3 AHI: attempted to reuse an in-flight buffer");
    }

    req->ahir_Std.io_Message.mn_Node.ln_Pri = 60;
    req->ahir_Std.io_Data = (APTR)buffer;
    req->ahir_Std.io_Length = buflen;
    req->ahir_Std.io_Offset = 0;
    req->ahir_Std.io_Command = CMD_WRITE;
    req->ahir_Frequency = device->spec.freq;
    req->ahir_Volume = 0x10000L;
    req->ahir_Type = data->ahi_type;
    req->ahir_Position = 0x8000L;

    /*
     * AHI's linked writes provide gapless double buffering. Link to the
     * previous buffer only while that request is genuinely still in flight.
     */
    req->ahir_Link = data->in_flight[previous_slot]
                     ? data->request[previous_slot]
                     : NULL;

    SendIO((struct IORequest *)req);
    data->in_flight[slot] = true;
    data->last_submitted = slot;
    data->playing++;
    data->current = previous_slot;

    return true;
}

static void OS3_CloseDevice(SDL_AudioDevice *device)
{
    OS3AudioData *data = (OS3AudioData *)device->hidden;
    if (!data) return;

    /*
     * Normally ThreadDeinit already closed AHI. Keep this defensive for
     * partially-created devices and failure paths.
     */
    OS3_CloseAHI(data);

    if (data->buffer[0]) FreeVec(data->buffer[0]);
    if (data->buffer[1]) FreeVec(data->buffer[1]);
    SDL_free(data);
    device->hidden = NULL;
}

static void OS3_FreeDeviceHandle(SDL_AudioDevice *device)
{
    SDL_free(device->handle);
    device->handle = NULL;
}

static bool OS3_Init(SDL_AudioDriverImpl *impl)
{
    if (!OS3_AudioAvailable()) {
        return SDL_SetError("AmigaOS3: AHI is not available");
    }

    impl->DetectDevices = OS3_DetectDevices;
    impl->OpenDevice = OS3_OpenDevice;
    impl->ThreadInit = OS3_ThreadInit;
    impl->ThreadDeinit = OS3_ThreadDeinit;
    impl->WaitDevice = OS3_WaitDevice;
    impl->PlayDevice = OS3_PlayDevice;
    impl->GetDeviceBuf = OS3_GetDeviceBuf;
    impl->CloseDevice = OS3_CloseDevice;
    impl->FreeDeviceHandle = OS3_FreeDeviceHandle;

    impl->ProvidesOwnCallbackThread = false;
    impl->HasRecordingSupport = false;
    impl->OnlyHasDefaultPlaybackDevice = true;
    impl->OnlyHasDefaultRecordingDevice = false;

    return true;
}

AudioBootStrap AMIGAOS3AUDIO_bootstrap = {
    "ahi", "AmigaOS3 AHI audio", OS3_Init, false, false
};

#endif
