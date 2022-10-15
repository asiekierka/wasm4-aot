#include <nds/arm9/cache.h>
#include <nds/fifocommon.h>
#include <stdint.h>
#include <stdio.h>
#include <nds.h>
#include "apu/apu.h"
#include "platform.h"

// TODO: Accurate noise

static int8_t sample_triangle[32] = {
    -128, -112, -96, -80, -64, -48, -32, -16, 0, 15, 31, 47, 63, 79, 95, 111, 127, 111, 95, 79, 63, 47, 31, 15, 0, -16, -32, -48, -64, -80, -96, -112,
};
static apu_initial_data_t apu_initial_data;
extern apu_time_t apu_time;
void platform_apu_init(void) {
    
    apu_initial_data.sample_triangle = sample_triangle;
    apu_initial_data.channels = w4_apuGetChannel(0);

    DC_FlushRange(&apu_initial_data, sizeof(apu_initial_data));
    fifoSendAddress(FIFO_USER_01, &apu_initial_data);
}

void platform_apu_update(void) {
    DC_FlushRange(apu_initial_data.channels, sizeof(apu_channel) * 4);
    fifoSendValue32(FIFO_USER_01, apu_time);

    apu_time += (PLATFORM_APU_SAMPLE_RATE / 60);
}