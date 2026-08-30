#ifndef SDL_os3audio_h_
#define SDL_os3audio_h_

#include <exec/types.h>
#include <exec/ports.h>
#include <devices/ahi.h>

#include "../SDL_sysaudio.h"

typedef struct SDL_PrivateAudioData
{
    struct MsgPort *reply_port;
    struct AHIRequest *request[2];
    struct AHIRequest *link;
    Uint8 *buffer[2];
    int current;
    int playing;
    bool in_flight[2];
    int last_submitted;
    int buffer_size;
    ULONG ahi_type;
    bool device_open;
    bool thread_open_failed;
} OS3AudioData;

#endif
