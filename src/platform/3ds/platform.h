#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PLATFORM_APU_SAMPLE_RATE 44100
#define PLATFORM_APU_64BIT_TIME 1
// #define DEBUG
#define PLATFORM_HAS_APU 1
// #define PLATFORM_HAS_DISK 1

void platform_init(void);
bool platform_update(void);
void platform_draw(void);
void platform_deinit(void);

#ifdef DEBUG
#define debug_printf iprintf
#endif