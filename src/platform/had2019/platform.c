#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "build_config.h"
#include "platform.h"
#include "runtime.h"

#define FB_WIDTH 480
#define FB_HEIGHT 320
#define PAL_OFFSET_COARSE 16
#define PAL_OFFSET_FINE 1

w4_Memory w4_memory;
uint8_t *fbmem;
uint32_t cur_vbl_ctr;
static uint8_t held_keys;
static uint32_t last_draw = 0;

void wait_for_vblank(uint32_t cur_vbl_ctr) {
    while (GFX_REG(GFX_VBLCTR_REG) <= cur_vbl_ctr);
}

void platform_init(void) {
    // Open framebuffer console for debugging
    // (prepared by IPL, uses tile layer A)
    // FILE *f;
    // f=fopen("/dev/console", "w");
    // setvbuf(f, NULL, _IONBF, 0); // make console line unbuffered
    // fprintf(f, "WASM4-AOT platform says hello!");

    held_keys = 0;

    printf("wasm4-aot: Hello, world!\n");

    // Disable all layers, set soft gray color as a background
    GFX_REG(GFX_BGNDCOL_REG)=0xff00ff;
    GFX_REG(GFX_LAYEREN_REG)=0;

    // Allocate framebuffer memory
    fbmem=calloc(FB_HEIGHT, FB_WIDTH/2);
    printf("wasm4-aot: framebuffer allocated @ 0x%p\n", fbmem);
    // Configure graphics pipeline:
    // - palette entries offset (we're leaving first 16 colors to IPL)
    // - framebuffer width
    GFX_REG(GFX_FBPITCH_REG)=(PAL_OFFSET_COARSE<<GFX_FBPITCH_PAL_OFF)|(480<<GFX_FBPITCH_PITCH_OFF);
    // - framebuffer layer address
    GFX_REG(GFX_FBADDR_REG)=((uint32_t)fbmem);

    // Clear the framebuffer
    for (int i = 0; i < ((FB_WIDTH/2) * FB_HEIGHT); i++) {
        fbmem[i] = 0;
    }

    // Reenable tile A and framebuffer (4-bit) layers
    GFX_REG(GFX_LAYEREN_REG)=GFX_LAYEREN_FB|GFX_LAYEREN_TILEA;
    cache_flush(fbmem, fbmem + (FB_WIDTH/2) * FB_HEIGHT);

    cur_vbl_ctr = GFX_REG(GFX_VBLCTR_REG);
}

bool platform_update(void) {
    uint32_t buttons = MISC_REG(MISC_BTN_REG);
    uint8_t mask = 0;
    if (buttons & BUTTON_UP)    mask |= W4_BUTTON_UP;
    if (buttons & BUTTON_DOWN)  mask |= W4_BUTTON_DOWN;
    if (buttons & BUTTON_LEFT)  mask |= W4_BUTTON_LEFT;
    if (buttons & BUTTON_RIGHT) mask |= W4_BUTTON_RIGHT;
    if (buttons & BUTTON_A)     mask |= W4_BUTTON_Z;
    if (buttons & BUTTON_B)     mask |= W4_BUTTON_X;
    if (buttons & BUTTON_START) return false;
    w4_runtimeSetGamepad(0, mask);
    return true;
}

#define PALMEM_ORDER(x) ((x & 0xFF00) | ((x >> 16) & 0xFF) | ((x << 16) & 0xFF0000) | 0xFF000000)

void platform_draw(void) {
    // Save current frame
    //cur_vbl_ctr = GFX_REG(GFX_VBLCTR_REG);

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
        uint8_t *fbline_odd = fbmem + ((FB_WIDTH/2) * ((y<<1) + 0)) + (80/2);
        uint8_t *fbline_even = fbmem + ((FB_WIDTH/2) * ((y<<1) + 1)) + (80/2);
        for (int x = 0; x < 160; x+=4, n++) {
            uint8_t quartet = w4_memory.framebuffer[n];
            // get 4x 2-bit pixels from WASM4 framebuffer
            int dot0 = (quartet & 0b00000011) >> 0;
            int dot1 = (quartet & 0b00001100) >> 2;
            int dot2 = (quartet & 0b00110000) >> 4;
            int dot3 = (quartet & 0b11000000) >> 6;

            // one byte of framebuffer = 2x 4-bit pixels
            *fbline_odd++ = (dot0+PAL_OFFSET_FINE) << 4 | (dot0+PAL_OFFSET_FINE);
            *fbline_odd++ = (dot1+PAL_OFFSET_FINE) << 4 | (dot1+PAL_OFFSET_FINE);
            *fbline_odd++ = (dot2+PAL_OFFSET_FINE) << 4 | (dot2+PAL_OFFSET_FINE);
            *fbline_odd++ = (dot3+PAL_OFFSET_FINE) << 4 | (dot3+PAL_OFFSET_FINE);
            *fbline_even++ = (dot0+PAL_OFFSET_FINE) << 4 | (dot0+PAL_OFFSET_FINE);
            *fbline_even++ = (dot1+PAL_OFFSET_FINE) << 4 | (dot1+PAL_OFFSET_FINE);
            *fbline_even++ = (dot2+PAL_OFFSET_FINE) << 4 | (dot2+PAL_OFFSET_FINE);
            *fbline_even++ = (dot3+PAL_OFFSET_FINE) << 4 | (dot3+PAL_OFFSET_FINE);
        }
        cache_flush(fbline_odd, fbline_even + (FB_WIDTH/2));
    }

    //cache_flush(fbmem, fbmem + (FB_WIDTH/2) * FB_HEIGHT);

    //wait_for_vblank(cur_vbl_ctr);
}

void platform_deinit(void) {
    GFX_REG(GFX_BGNDCOL_REG) = 0x202020;
    GFX_REG(GFX_LAYEREN_REG) = 0;
    GFX_REG(GFX_FBADDR_REG) = 0;
    free(fbmem);
    printf("wasm4-aot: Goodbye!\n");
}
