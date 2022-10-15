#include <stdint.h>
#include <nds.h>
#include "platform.h"
#include "runtime.h"

w4_Memory w4_memory;

DTCM_DATA
const uint32_t bpp_2_8_table[256] = {
#include "fb_table.inc"
};

DTCM_BSS
static uint16_t held_keys, vbl_ticks;

void platform_irq_vblank() {
    vbl_ticks++;
}

#ifdef DEBUG
extern PrintConsole defaultConsole;
#endif

void platform_apu_init(void);
void platform_apu_update(void);

void platform_init(void) {
    vramSetBankE(VRAM_E_MAIN_BG);

	irqSet(IRQ_VBLANK, platform_irq_vblank);
    irqEnable(IRQ_VBLANK);

    videoSetMode(MODE_5_2D | DISPLAY_BG2_ACTIVE);
    REG_BG2CNT = BG_BMP8_256x256 | BG_BMP_BASE(0);
    REG_BG2PA = 256;
    REG_BG2PB = 0;
    REG_BG2PC = 0;
    REG_BG2PD = 256;
    REG_BG2X = -((256 - 160) << 7);
    REG_BG2Y = -((192 - 160) << 7);
    
#ifdef DEBUG
    videoSetModeSub(MODE_0_2D);
    vramSetBankH(VRAM_H_SUB_BG);
    vramSetBankI(VRAM_I_SUB_BG_0x06208000);

    consoleInit(NULL, defaultConsole.bgLayer, BgType_Text4bpp, BgSize_T_256x256, defaultConsole.mapBase, defaultConsole.gfxBase, false, true);
#else
    videoSetModeSub(MODE_0_2D);
#endif

#ifdef PLATFORM_HAS_APU
    platform_apu_init();
#endif
}

bool platform_update(void) {
    scanKeys();
    if (keysDown() & (KEY_L | KEY_R)) {
        lcdSwap();
    }
    uint32_t held_keys = keysDown() | keysHeld();
    uint8_t w4_held_keys =
        ((held_keys & KEY_UP) ? W4_BUTTON_UP : 0)
        | ((held_keys & KEY_DOWN) ? W4_BUTTON_DOWN : 0)
        | ((held_keys & KEY_RIGHT) ? W4_BUTTON_RIGHT : 0)
        | ((held_keys & KEY_LEFT) ? W4_BUTTON_LEFT : 0)
        | ((held_keys & KEY_B) ? W4_BUTTON_X : 0)
        | ((held_keys & KEY_A) ? W4_BUTTON_Z : 0);
    w4_runtimeSetGamepad(0, w4_held_keys);

    if (held_keys & KEY_TOUCH) {
        touchPosition touchPos;
        touchRead(&touchPos);

        int x1 = (256 - W4_WIDTH) >> 1;
        int y1 = (192 - W4_HEIGHT) >> 1;
        
        w4_runtimeSetMouse(touchPos.px - x1, touchPos.py - y1, W4_MOUSE_LEFT);
    } else {
        w4_runtimeClearMouse();
    }

    return true;
}

ITCM_CODE
void platform_draw(void) {
#ifdef PLATFORM_HAS_APU
    // can be run asynchronously - framebuffer blitting takes a fair few CPU cycles.
    platform_apu_update();
#endif

    uint16_t *bg_pal = BG_PALETTE + 0x10;
    for (int i = 0; i < 4; i++, bg_pal++) {
        uint16_t red = (w4_memory.palette[i] >> 19) & 0x1F;
        uint16_t green = (w4_memory.palette[i] >> 11) & 0x1F;
        uint16_t blue = (w4_memory.palette[i] >> 3) & 0x1F;
        *bg_pal = red | (green << 5) | (blue << 10) | 0x8000;
    } 

    // full framebuffer blit here is slow...
    uint32_t *vram = (uint32_t*) BG_GFX;
    uint8_t *src = w4_memory.framebuffer;
    for (int y = 0; y < W4_HEIGHT; y++, vram += ((256 - 160) >> 2)) {
        for (int x = 0; x < W4_WIDTH; x += 4, vram++, src++) {
            *vram = bpp_2_8_table[*src];
        }
    }

    if (vbl_ticks == 0) {
        swiWaitForVBlank();
    }
    int oldIME = enterCriticalSection();
    vbl_ticks = 0;
    leaveCriticalSection(oldIME);
}

void platform_deinit(void) {
    
}

void* platform_init_alloc(uint32_t len) {
    return sbrk(len);
}