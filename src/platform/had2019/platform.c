#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "build_config.h"
#include "platform.h"
#include "runtime.h"

#define FB_WIDTH 480
#define FB_HEIGHT 320
#define FB_LINE_PITCH (FB_WIDTH / 2) // in bytes, 480 pixels wide, 4bpp
#define PAL_OFFSET_COARSE 16
#define PAL_OFFSET_FINE 0

#define WORKTILE_OFFSET 272 // 272 first tiles preloaded by IPL, to be reused for debug print
#define WORKTILE_MEM_OFFSET (WORKTILE_OFFSET * 64) // memory offset in bytes

w4_Memory w4_memory;
uint8_t *fbmem;
static uint8_t held_keys;
static uint32_t last_draw = 0;

extern uint32_t GFXPAL[];
extern uint32_t GFXTILES[];
extern uint32_t GFXTILEMAPA[];
extern uint32_t GFXTILEMAPB[];
extern uint32_t GFXSPRITES[];
extern uint32_t GFXCOPPEROPS[];

const uint32_t bpp_2_8_table[256] = {
#include "fb_table.inc"
};

void wait_for_vblank(uint32_t cur_vbl_ctr) {
    while (GFX_REG(GFX_VBLCTR_REG) <= cur_vbl_ctr);
}

static inline void tile_b_translate(uint16_t dx, uint16_t dy) {
    GFX_REG(GFX_TILEB_OFF) = (dy << 16) + (dx << 0);
}

static inline void draw_quadpixel_to_tile(uint32_t quad, uint16_t qx, uint16_t y) {
    uint32_t tile_nb = (y * 10 / 16) + (qx / 4);
    uint32_t tile_qp = ((y & 0x000f) << 4) + (qx & 0x0003);
    GFXTILES[WORKTILE_MEM_OFFSET + (tile_nb * 16) + tile_qp] = quad;
}

void platform_init(void) {
    // Open framebuffer console for debugging
    // (prepared by IPL, uses tile layer A)
    // FILE *f;
    // f=fopen("/dev/console", "w");
    // setvbuf(f, NULL, _IONBF, 0); // make console line unbuffered
    // fprintf(f, "WASM4-AOT platform says hello!");

    //printf("wasm4-aot: Hello, world!\n");

    held_keys = 0;

    // Disable all layers, set soft gray color as a background
    GFX_REG(GFX_BGNDCOL_REG) = 0x202020;
    GFX_REG(GFX_LAYEREN_REG) = 0;

    // Allocate framebuffer memory
    fbmem = calloc(FB_HEIGHT, FB_LINE_PITCH);

    // Configure framebuffer:
    // - palette entries offset (we're leaving first 16 colors to IPL and next 4 to WASM4)
    // - framebuffer width for static background
    GFX_REG(GFX_FBPITCH_REG) = ((PAL_OFFSET_COARSE+4) << GFX_FBPITCH_PAL_OFF) | (FB_LINE_PITCH << (GFX_FBPITCH_PITCH_OFF + 1));
    // - framebuffer layer address
    GFX_REG(GFX_FBADDR_REG) = ((uint32_t)fbmem);

    // Clear the framebuffer
    for (int i = 0; i < ((FB_LINE_PITCH) * FB_HEIGHT); i++) {
        fbmem[i] = 0;
    }

    // TODO: Custom background on framebuffer layer

    // Clear tile B layer
    for (int x=0; x<64*64; x++) GFXTILEMAPB[x]=0;

    // Fill tile B with consecutive tiles
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            GFXTILEMAPB[(y * 64) + x + 3] = WORKTILE_OFFSET + ((y * 10) + x);
        }
    }

    // Move tile B layer 8 pixels left (pre-scaling)
    tile_b_translate(64*8, 0);

    // Scale tile B layer 2x
    GFX_REG(GFX_TILEB_INC_COL) =        (0 << 16) + ((64 / 2) & 0xffff);
    GFX_REG(GFX_TILEB_INC_ROW) = ((64 / 2) << 16) +        (0 & 0xffff);

    // Reenable framebuffer (4-bit) and tile B layers
    GFX_REG(GFX_LAYEREN_REG) = GFX_LAYEREN_FB | GFX_LAYEREN_TILEB;
    cache_flush(fbmem, fbmem + (FB_LINE_PITCH * FB_HEIGHT));
}

bool platform_update(void) {
    uint32_t buttons = MISC_REG(MISC_BTN_REG);
    uint8_t mask = 0;
    if (buttons & BUTTON_UP)    mask |= W4_BUTTON_UP;
    if (buttons & BUTTON_DOWN)  mask |= W4_BUTTON_DOWN;
    if (buttons & BUTTON_LEFT)  mask |= W4_BUTTON_LEFT;
    if (buttons & BUTTON_RIGHT) mask |= W4_BUTTON_RIGHT;
    if (buttons & BUTTON_A)     mask |= W4_BUTTON_X;
    if (buttons & BUTTON_B)     mask |= W4_BUTTON_Z;
    if (buttons & BUTTON_START) return false;
    w4_runtimeSetGamepad(0, mask);
    return true;
}

#define PALMEM_ORDER(x) ((x & 0xFF00) | ((x >> 16) & 0xFF) | ((x << 16) & 0xFF0000) | 0xFF000000)

void platform_draw(void) {
    // Save current vblank
    //uint32_t cur_vbl_ctr = GFX_REG(GFX_VBLCTR_REG);

    // Reload palette
    uint32_t *w4_pal = w4_memory.palette;
    GFXPAL[PAL_OFFSET_COARSE+PAL_OFFSET_FINE+0] = PALMEM_ORDER(w4_pal[0]);
    GFXPAL[PAL_OFFSET_COARSE+PAL_OFFSET_FINE+1] = PALMEM_ORDER(w4_pal[1]);
    GFXPAL[PAL_OFFSET_COARSE+PAL_OFFSET_FINE+2] = PALMEM_ORDER(w4_pal[2]);
    GFXPAL[PAL_OFFSET_COARSE+PAL_OFFSET_FINE+3] = PALMEM_ORDER(w4_pal[3]);
    cache_flush(&GFXPAL[PAL_OFFSET_COARSE], &GFXPAL[PAL_OFFSET_COARSE+PAL_OFFSET_FINE+4]);

    // Draw pixels
    int n = 0;
    for (int y = 0; y < 160; y++) {
        for (int qx = 0; qx < 40; qx++, n++) {
            uint8_t w4_pixels = w4_memory.framebuffer[n];
            uint32_t tile_pixels = bpp_2_8_table[w4_pixels];
            draw_quadpixel_to_tile(tile_pixels, qx, y);
        }
    }

    cache_flush(&GFXTILES[WORKTILE_OFFSET], &GFXTILES[WORKTILE_OFFSET + 100]);

    // Wait until next frame (?)
    //wait_for_vblank(cur_vbl_ctr);
}

void platform_deinit(void) {
    GFX_REG(GFX_BGNDCOL_REG) = 0x202020;
    GFX_REG(GFX_LAYEREN_REG) = 0;
    GFX_REG(GFX_FBADDR_REG) = 0;
    free(fbmem);
}
