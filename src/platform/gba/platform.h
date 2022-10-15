#include <stdint.h>

#define PLATFORM_HAS_DISK 1
// #define PLATFORM_HAS_DRAW_PARTIAL 1

void platform_init(void);
void platform_update(void);
void platform_draw(void);
// void platform_draw_partial(int x, int y, int width, int height);
void platform_deinit(void);