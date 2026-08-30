#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    SDL_AudioSpec spec;
    SDL_AudioStream *stream;
    Sint16 *samples;
    const int freq = 22050;
    const int seconds = 3;
    const int frames = freq * seconds;
    int i;

    if (!SDL_Init(SDL_INIT_AUDIO)) {
        printf("SDL_Init audio failed: %s\n", SDL_GetError());
        return 1;
    }

    printf("audio driver: %s\n", SDL_GetCurrentAudioDriver());
    printf("source spec: 22050 Hz S16BE stereo\n");

    SDL_zero(spec);
    spec.freq = freq;
    spec.format = SDL_AUDIO_S16BE;
    spec.channels = 2;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                       &spec, NULL, NULL);
    if (!stream) {
        printf("SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 2;
    }

    samples = (Sint16 *)SDL_malloc((size_t)frames * 2 * sizeof(Sint16));
    if (!samples) {
        SDL_DestroyAudioStream(stream);
        SDL_Quit();
        return 3;
    }

    for (i = 0; i < frames; ++i) {
        double phase = ((double)i * 440.0 * 2.0 * M_PI) / (double)freq;
        Sint16 s = (Sint16)(SDL_sin(phase) * 12000.0);
        samples[i * 2 + 0] = s;
        samples[i * 2 + 1] = s;
    }

    if (!SDL_PutAudioStreamData(stream, samples,
                                frames * 2 * (int)sizeof(Sint16))) {
        printf("SDL_PutAudioStreamData failed: %s\n", SDL_GetError());
        SDL_free(samples);
        SDL_DestroyAudioStream(stream);
        SDL_Quit();
        return 4;
    }
    SDL_free(samples);

    if (!SDL_ResumeAudioStreamDevice(stream)) {
        printf("SDL_ResumeAudioStreamDevice failed: %s\n", SDL_GetError());
        SDL_DestroyAudioStream(stream);
        SDL_Quit();
        return 5;
    }

    printf("Playing 440 Hz SDL_sin() tone for about 3 seconds through AHI...\n");

    /*
     * Do not use SDL_GetAudioStreamQueued()==0 as a shutdown condition here.
     * This smoke test only needs to prove playback and clean teardown.
     */
    SDL_Delay(3500);

    SDL_DestroyAudioStream(stream);
    SDL_Quit();

    printf("audio: RESULT OK\n");
    return 0;
}
