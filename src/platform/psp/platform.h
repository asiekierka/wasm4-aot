#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PLATFORM_APU_SAMPLE_RATE 44100
#define PLATFORM_HAS_APU 1

void platform_init(void);
bool platform_update(void);
void platform_draw(void);
void platform_deinit(void);