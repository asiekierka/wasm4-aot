#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PLATFORM_HAS_DISK 1
// #define PLATFORM_HAS_DRAW_PARTIAL 1
#define PLATFORM_HAS_INIT_ALLOC 1

void platform_init(void);
bool platform_update(void);
void platform_draw(void);
// void platform_draw_partial(int x, int y, int width, int height);
void platform_deinit(void);
void* platform_init_alloc(uint32_t len);