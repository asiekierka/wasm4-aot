#pragma once

#include <stdint.h>
#include <stddef.h>
#include "platform_shim.h"

#ifdef PLATFORM_APU_64BIT_TIME
typedef uint64_t apu_time_t;
#else
typedef uint32_t apu_time_t;
#endif

typedef struct {
    /** Starting frequency. */
    uint16_t freq1;

    /** Ending frequency, or zero for no frequency transition. */
    uint16_t freq2;

    /** Time the tone was started. */
    apu_time_t startTime;

    /** Time at the end of the attack period. */
    apu_time_t attackTime;

    /** Time at the end of the decay period. */
    apu_time_t decayTime;

    /** Time at the end of the sustain period. */
    apu_time_t sustainTime;

    /** Time the tone should end. */
    apu_time_t releaseTime;

    /** Sustain volume level. */
    int16_t sustainVolume;

     /** Peak volume level at the end of the attack phase. */
    int16_t peakVolume;

    /** Used for time tracking. */
    float phase;

    /** Tone panning. 0 = center, 1 = only left, 2 = only right. */
    uint8_t pan;

    union {
        struct {
            /** Duty cycle for pulse channels. */
            float dutyCycle;
        } pulse;

        struct {
            /** Noise generation state. */
            uint16_t seed;

            /** The last generated random number, either -1 or 1. */
            int16_t lastRandom;
        } noise;
    };
} apu_channel;

void w4_apuInit ();

void w4_apuTone (int frequency, int duration, int volume, int flags);

void w4_apuWriteSamples (int16_t* output, unsigned long frames);

const apu_channel* w4_apuGetChannel (int index);

uint16_t w4_apuGetCurrentFrequency (const apu_channel* channel);

int16_t w4_apuGetCurrentVolume (const apu_channel* channel);
