#include <exec/types.h>
#include <exec/ports.h>
#include <proto/exec.h>
#include <devices/ahi.h>
#include <stdio.h>
#include <stdlib.h>
#include <exec/memory.h>
#include <string.h>

static int play_buffer(struct AHIRequest *req,
                       void *buffer,
                       ULONG length,
                       ULONG frequency,
                       ULONG type,
                       const char *label)
{
    printf("Playing %s for 2 seconds...\n", label);

    req->ahir_Std.io_Message.mn_Node.ln_Pri = 60;
    req->ahir_Std.io_Command = CMD_WRITE;
    req->ahir_Std.io_Data = buffer;
    req->ahir_Std.io_Length = length;
    req->ahir_Std.io_Offset = 0;
    req->ahir_Frequency = frequency;
    req->ahir_Type = type;
    req->ahir_Volume = 0x10000;
    req->ahir_Position = 0x8000;
    req->ahir_Link = NULL;

    DoIO((struct IORequest *)req);

    printf("  io_Error=%d\n", (int)req->ahir_Std.io_Error);
    return req->ahir_Std.io_Error;
}

int main(void)
{
    struct MsgPort *port = NULL;
    struct AHIRequest *req = NULL;

    const int freq = 22050;
    const int seconds = 2;
    const int frames = freq * seconds;
    const int half_period = freq / (2 * 440);

    signed char *m8 = NULL;
    signed char *s8 = NULL;
    short *m16 = NULL;
    short *s16 = NULL;

    int i;
    int rc = 1;

    port = CreateMsgPort();
    if (!port) {
        printf("CreateMsgPort failed\n");
        goto done;
    }

    req = (struct AHIRequest *)CreateIORequest(port, sizeof(struct AHIRequest));
    if (!req) {
        printf("CreateIORequest failed\n");
        goto done;
    }

    req->ahir_Version = 4;

    if (OpenDevice(AHINAME, AHI_DEFAULT_UNIT,
                   (struct IORequest *)req, 0) != 0) {
        printf("OpenDevice(%s) failed\n", AHINAME);
        goto done;
    }

    m8  = (signed char *)AllocVec((ULONG)frames, MEMF_PUBLIC);
    s8  = (signed char *)AllocVec((ULONG)(frames * 2), MEMF_PUBLIC);
    m16 = (short *)AllocVec((ULONG)(frames * sizeof(short)), MEMF_PUBLIC);
    s16 = (short *)AllocVec((ULONG)(frames * 2 * sizeof(short)), MEMF_PUBLIC);

    if (!m8 || !s8 || !m16 || !s16) {
        printf("malloc failed\n");
        goto close_device;
    }

    for (i = 0; i < frames; ++i) {
        int high = ((i / half_period) & 1) == 0;
        signed char v8 = high ? 110 : -110;
        short v16 = high ? 28000 : -28000;

        m8[i] = v8;

        s8[i * 2 + 0] = v8;
        s8[i * 2 + 1] = v8;

        m16[i] = v16;

        s16[i * 2 + 0] = v16;
        s16[i * 2 + 1] = v16;
    }

    printf("AHI format diagnostic\n");
    printf("Each test is a loud 440 Hz square wave.\n");
    printf("There should be four clearly audible 2-second tones.\n\n");

    if (play_buffer(req, m8, frames,
                    freq, AHIST_M8S,
                    "8-bit mono (AHIST_M8S)") != 0) {
        goto close_device;
    }

    if (play_buffer(req, s8, frames * 2,
                    freq, AHIST_S8S,
                    "8-bit stereo (AHIST_S8S)") != 0) {
        goto close_device;
    }

    if (play_buffer(req, m16, frames * sizeof(short),
                    freq, AHIST_M16S,
                    "16-bit mono (AHIST_M16S)") != 0) {
        goto close_device;
    }

    if (play_buffer(req, s16, frames * 2 * sizeof(short),
                    freq, AHIST_S16S,
                    "16-bit stereo (AHIST_S16S)") != 0) {
        goto close_device;
    }

    printf("\nAHI format diagnostic finished.\n");
    rc = 0;

close_device:
    if (m8) FreeVec(m8);
    if (s8) FreeVec(s8);
    if (m16) FreeVec(m16);
    if (s16) FreeVec(s16);
    CloseDevice((struct IORequest *)req);

done:
    if (req) {
        DeleteIORequest((struct IORequest *)req);
    }
    if (port) {
        DeleteMsgPort(port);
    }

    return rc;
}
