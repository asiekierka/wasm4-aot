#include <stdint.h>
#include <tonc.h>
#include <tonc_memdef.h>
#include "runtime.h"

EWRAM_BSS
w4_Memory w4_memory;

IWRAM_DATA
const uint32_t bpp_2_8_table[256] = {
#include "fb_table.inc"
};

void platform_init(void) {
    REG_BG2CNT = 0;
    REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;
}

void platform_update(void) {
    key_poll();
    uint32_t held_keys = key_held(-1);
    uint8_t w4_held_keys =
        ((held_keys & KEY_UP) ? W4_BUTTON_UP : 0)
         |((held_keys & KEY_DOWN) ? W4_BUTTON_DOWN : 0)
        | ((held_keys & KEY_RIGHT) ? W4_BUTTON_RIGHT : 0)
        | ((held_keys & KEY_LEFT) ? W4_BUTTON_LEFT : 0)
        | ((held_keys & KEY_B) ? W4_BUTTON_X : 0)
        | ((held_keys & KEY_A) ? W4_BUTTON_Z : 0);
    w4_runtimeSetGamepad(0, w4_held_keys);
}

void platform_draw(void) {
    uint16_t *bg_pal = (uint16_t*) (MEM_PAL + (0x10 << 1));
    for (int i = 0; i < 4; i++, bg_pal++) {
        uint16_t red = (w4_memory.palette[i] >> 19) & 0x1F;
        uint16_t green = (w4_memory.palette[i] >> 11) & 0x1F;
        uint16_t blue = (w4_memory.palette[i] >> 3) & 0x1F;
        *bg_pal = red | (green << 5) | (blue << 10);
    } 

    // full framebuffer blit here is slow...
    uint32_t *vram = (uint32_t*) (MEM_VRAM + 40);
    uint8_t *src = w4_memory.framebuffer;
    for (int y = 0; y < W4_HEIGHT; y++, vram += ((240 - 160) >> 2)) {
        for (int x = 0; x < W4_WIDTH; x += 4, vram++, src++) {
            *vram = bpp_2_8_table[*src];
        }
    }
}

void platform_draw_partial(int x, int y, int width, int height) {
    int x1 = x>>2;
    int x2 = (x+width-1+3)>>2;
    int xd = x2 - x1;
    int xd60 = 60 - xd;
    int xd40 = 40 - xd;

    // full framebuffer blit here is slow...
    uint32_t *vram = (uint32_t*) (MEM_VRAM + 40 + (y * 240) + (x1 * 4));
    uint8_t *src = w4_memory.framebuffer + (y * 40) + x1;
    for (int y = 0; y < height; y++, vram += xd60, src += xd40) {
        for (int x = 0; x < xd; x++, vram++, src++) {
            *vram = bpp_2_8_table[*src];
        }
    }
}

void platform_deinit(void) {
    
}