#include <stdint.h>
#include <stdlib.h>
#include "platform.h"
#include "runtime.h"

w4_Memory w4_memory;

#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspaudio.h>
#include <pspaudiolib.h>
#include <psppower.h>
#include <psputility.h>

PSP_MODULE_INFO("wasm4aot", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4096);
PSP_MAIN_THREAD_STACK_SIZE_KB(256);

static u32 __attribute__((aligned(16))) gu_clut4[8];
static u32 __attribute__((aligned(16))) gu_list[262144];
static u16 __attribute__((aligned(16))) gu_tex[256 * 64];

const uint16_t bpp_2_4_table[256] = {
#include "fb_table.inc"
};

#ifdef PLATFORM_HAS_APU
#include "apu.h"

void psp_audio_callback(void *stream, unsigned int len, void *userdata) {
    w4_apuWriteSamples((int16_t*) stream, len);
}
#endif

int psp_exit_cb(int a1, int a2, void *a3) {
	sceKernelExitGame();
	return 0;
}

int psp_exit_thread(SceSize args, void *argp) {
	int cbid = sceKernelCreateCallback("exit callback", (void*) psp_exit_cb, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

void platform_init(void) {
	{
		int thid = sceKernelCreateThread("exit handler", psp_exit_thread, 0x1E, 0x800, PSP_THREAD_ATTR_USER, NULL);
		if (thid >= 0) {
			sceKernelStartThread(thid, 0, 0);
		} else {
			exit(1);
		}
	}

    sceGuInit();
    sceGuStart(GU_DIRECT, gu_list);
    sceGuDrawBuffer(GU_PSM_8888, NULL, 512);
    sceGuDispBuffer(480, 272, (void*) 0x88000, 512);
    sceGuDepthBuffer((void*) 0x110000, 512);
    sceGuOffset(2048 - 240, 2048 - 136);
    sceGuViewport(2048, 2048, 480, 272);
    sceGuDepthRange(0xC350, 0x2710);
    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuShadeModel(GU_FLAT);
    sceGuAlphaFunc(GU_GREATER, 0, 0xFF);
    sceGuEnable(GU_ALPHA_TEST);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_T4, 0, 0, 0);
    sceGuTexImage(0, 256, 256, 256, gu_tex);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexEnvColor(0x0);
    sceGuTexOffset(0.0f, 0.0f);
    sceGuTexScale(1.0f / 256.0f, 1.0f / 256.0f);
    sceGuTexWrap(GU_REPEAT, GU_REPEAT);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    sceGuFinish();
    sceGuSync(0, 0);
    sceGuDisplay(GU_TRUE);

#ifdef PLATFORM_HAS_APU    
	pspAudioInit();
	pspAudioSetChannelCallback(0, psp_audio_callback, NULL);
#endif

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
}

bool platform_update(void) {
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad, 1);

    uint8_t w4_held_keys =
        ((pad.Buttons & PSP_CTRL_UP) ? W4_BUTTON_UP : 0)
        | ((pad.Buttons & PSP_CTRL_DOWN) ? W4_BUTTON_DOWN : 0)
        | ((pad.Buttons & PSP_CTRL_RIGHT) ? W4_BUTTON_RIGHT : 0)
        | ((pad.Buttons & PSP_CTRL_LEFT) ? W4_BUTTON_LEFT : 0)
        | ((pad.Buttons & PSP_CTRL_CIRCLE) ? W4_BUTTON_X : 0)
        | ((pad.Buttons & PSP_CTRL_CROSS) ? W4_BUTTON_Z : 0);
    w4_runtimeSetGamepad(0, w4_held_keys);

    return true;
}

typedef struct {
	u16 u, v;
	u32 color;
	u16 x, y, z, v_pad;
} point_fg;

void platform_draw(void) {
    uint16_t *vram = (uint16_t*) gu_tex;
    uint8_t *src = w4_memory.framebuffer;
    for (int y = 0; y < W4_HEIGHT; y++, vram += ((256 - 160) >> 2)) {
        for (int x = 0; x < W4_WIDTH; x += 4, vram++, src++) {
            *vram = bpp_2_4_table[*src];
        }
    }

    for (int i = 0; i < 4; i++) {
        gu_clut4[i] = (w4_memory.palette[i] & 0xFF00) | ((w4_memory.palette[i] >> 16) & 0xFF) | ((w4_memory.palette[i] << 16) & 0xFF0000) | 0xFF000000;
    } 

    // sceKernelDcacheWritebackInvalidateRange(gu_tex, sizeof(gu_tex));
    // sceKernelDcacheWritebackInvalidateRange(gu_clut4, sizeof(gu_clut4));

	sceGuStart(GU_DIRECT, gu_list);

    sceGuClutMode(GU_PSM_8888, 0, 0x0F, 0);
    sceGuClutLoad(2, gu_clut4);

    point_fg *fg_cells = sceGuGetMemory(sizeof(point_fg) * 2);
    fg_cells[0].x = (480 - W4_WIDTH) >> 1;
    fg_cells[0].y = (272 - W4_HEIGHT) >> 1;
    fg_cells[0].z = 0;
    fg_cells[0].u = 0;
    fg_cells[0].v = 0;
    fg_cells[0].color = 0xFFFFFFFF;
    fg_cells[1].x = fg_cells[0].x + W4_WIDTH;
    fg_cells[1].y = fg_cells[0].y + W4_HEIGHT;
    fg_cells[1].z = 0;
    fg_cells[1].u = W4_WIDTH;
    fg_cells[1].v = W4_HEIGHT;
    fg_cells[1].color = 0xFFFFFFFF;

	sceGumDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, fg_cells);

	sceGuFinish();
	sceGuSync(0, 0);

	sceDisplayWaitVblankStartCB();
	sceGuSwapBuffers();
}

void platform_deinit(void) {
	sceGuDisplay(GU_FALSE);
	sceGuTerm();
	sceKernelExitGame();
}