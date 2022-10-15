#include <stdint.h>

#define PLATFORM_HAS_DISK 1

void platform_init(void);
void platform_update(void);
void platform_draw(void);
void platform_deinit(void);