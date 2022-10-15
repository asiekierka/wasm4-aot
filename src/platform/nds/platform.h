#pragma once

#include <stdint.h>

#define PLATFORM_APU_SAMPLE_RATE 120
// #define DEBUG
#define PLATFORM_HAS_APU 1
// #define PLATFORM_HAS_DISK 1
// #define PLATFORM_HAS_DRAW_PARTIAL 1
#define PLATFORM_HAS_INIT_ALLOC 1

void platform_init(void);
void platform_update(void);
void platform_draw(void);
void platform_deinit(void);
void* platform_init_alloc(uint32_t len);

#ifdef DEBUG
#define debug_printf iprintf
#endif

#ifdef PLATFORM_HAS_APU
#include "apu.h"

typedef struct {
    uint8_t *sample_triangle;
    const void *channels;
} apu_initial_data_t;
#endif