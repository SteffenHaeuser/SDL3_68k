#include <exec/types.h>
#include <exec/ports.h>
#include <proto/exec.h>
#include <devices/ahi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <exec/memory.h>

int main(void)
{
    struct MsgPort *port = NULL;
    struct AHIRequest *req = NULL;
    Sint16 *samples = NULL;
    const int freq = 44100;
    const int seconds = 3;
    const int frames = freq * seconds;
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
    if (OpenDevice(AHINAME, AHI_DEFAULT_UNIT, (struct IORequest *)req, 0) != 0) {
        printf("OpenDevice(%s) failed\n", AHINAME);
        goto done;
    }

    samples = (Sint16 *)malloc((size_t)frames * 2 * sizeof(Sint16));
    if (!samples) {
        printf("malloc failed\n");
        goto close_device;
    }

    for (i = 0; i < frames; ++i) {
        double phase = ((double)i * 440.0 * 2.0 * M_PI) / (double)freq;
        Sint16 s = (Sint16)(sin(phase) * 12000.0);
        samples[i * 2] = s;
        samples[i * 2 + 1] = s;
    }

    req->ahir_Std.io_Message.mn_Node.ln_Pri = 60;
    req->ahir_Std.io_Command = CMD_WRITE;
    req->ahir_Std.io_Data = samples;
    req->ahir_Std.io_Length = frames * 2 * sizeof(Sint16);
    req->ahir_Std.io_Offset = 0;
    req->ahir_Frequency = freq;
    req->ahir_Type = AHIST_S16S;
    req->ahir_Volume = 0x10000;
    req->ahir_Position = 0x8000;
    req->ahir_Link = NULL;

    printf("Sending one 3-second 440 Hz AHI request...\n");
    DoIO((struct IORequest *)req);
    printf("AHI io_Error=%d\n", (int)req->ahir_Std.io_Error);
    if (req->ahir_Std.io_Error == 0) {
        printf("direct AHI: RESULT OK\n");
        rc = 0;
    }

    if (samples) FreeVec(samples);

close_device:
    CloseDevice((struct IORequest *)req);
done:
    if (req) DeleteIORequest((struct IORequest *)req);
    if (port) DeleteMsgPort(port);
    return rc;
}
