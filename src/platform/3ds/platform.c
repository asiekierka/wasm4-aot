#include <stdbool.h>
#include <stdint.h>
#include <3ds.h>
#include "platform.h"
#include "runtime.h"

#include "grapefruit.h"
#include "shader_shbin.h"

w4_Memory w4_memory;

void platform_apu_init(void);
void platform_apu_deinit(void);

static C3D_Tex tex;
static u32 *texBuf;
static C3D_Mtx proj_top, proj_bottom;
static C3D_RenderTarget *target_top, *target_bottom;
static struct ctr_shader_data shader;
static bool drawOnBottomScreen;

void platform_init(void) {
    // system init
	osSetSpeedupEnable(1);

    // video init
    drawOnBottomScreen = false;

	C3D_TexEnv* texEnv;

	gfxInitDefault();
	gfxSet3D(false);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	target_top = C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, GPU_RB_DEPTH16);
	target_bottom = C3D_RenderTargetCreate(240, 320, GPU_RB_RGB8, GPU_RB_DEPTH16);
	C3D_RenderTargetClear(target_top, C3D_CLEAR_ALL, 0, 0);
	C3D_RenderTargetClear(target_bottom, C3D_CLEAR_ALL, 0, 0);
	C3D_RenderTargetSetOutput(target_top, GFX_TOP, GFX_LEFT,
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8));
	C3D_RenderTargetSetOutput(target_bottom, GFX_BOTTOM, GFX_LEFT,
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8));

	C3D_TexInitVRAM(&tex, 256, 256, GPU_RGBA8);
	texBuf = linearAlloc(256 * 256 * 4);

	ctr_init_shader(&shader, shader_shbin, shader_shbin_size);
	AttrInfo_AddLoader(&(shader.attr), 0, GPU_FLOAT, 3); // v0 = position
	AttrInfo_AddLoader(&(shader.attr), 1, GPU_FLOAT, 2); // v1 = texcoord
	ctr_bind_shader(&shader);

	Mtx_OrthoTilt(&proj_top, 0.0, 400.0, 0.0, 240.0, -1.0, 1.0, true);
	Mtx_OrthoTilt(&proj_bottom, 0.0, 320.0, 0.0, 240.0, -1.0, 1.0, true);

	texEnv = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(texEnv, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvOpRgb(texEnv, 0, 0, 0);
	C3D_TexEnvOpAlpha(texEnv, 0, 0, 0);
	C3D_TexEnvFunc(texEnv, C3D_Both, GPU_MODULATE);

	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    // audio init
#ifdef PLATFORM_HAS_APU
    platform_apu_init();
#endif
}

bool platform_update(void) {
    if (!aptMainLoop()) {
        return false;
    }

    scanKeys();
    uint32_t held_keys = keysDown() | keysHeld();
    uint8_t w4_held_keys =
        ((held_keys & KEY_UP) ? W4_BUTTON_UP : 0)
        | ((held_keys & KEY_DOWN) ? W4_BUTTON_DOWN : 0)
        | ((held_keys & KEY_RIGHT) ? W4_BUTTON_RIGHT : 0)
        | ((held_keys & KEY_LEFT) ? W4_BUTTON_LEFT : 0)
        | ((held_keys & KEY_B) ? W4_BUTTON_X : 0)
        | ((held_keys & KEY_A) ? W4_BUTTON_Z : 0);
    w4_runtimeSetGamepad(0, w4_held_keys);

    return true;
}

void platform_draw(void) {
    uint32_t palette[4];
    for (int i = 0; i < 4; i++) {
        palette[i] = (w4_memory.palette[i] << 8) | 0xFF;
    }

    uint32_t *dst = texBuf;
    uint8_t *src = w4_memory.framebuffer;

    for (int y = 0; y < W4_HEIGHT; y++, dst += (256 - W4_WIDTH)) {
        for (int x = 0; x < W4_WIDTH; x += 4) {
            uint8_t pixel = *(src++);
            *(dst++) = palette[pixel & 0x3]; pixel >>= 2;
            *(dst++) = palette[pixel & 0x3]; pixel >>= 2;
            *(dst++) = palette[pixel & 0x3]; pixel >>= 2;
            *(dst++) = palette[pixel & 0x3];
        }
    }

	GSPGPU_FlushDataCache(texBuf, 256 * 256 * 4);
	C3D_SyncDisplayTransfer(texBuf, GX_BUFFER_DIM(256, 256), tex.data, GX_BUFFER_DIM(tex.width, tex.height),
		(GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) |
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) |
		GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
	);

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C3D_FrameDrawOn(drawOnBottomScreen ? target_bottom : target_top);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shader.proj_loc, drawOnBottomScreen ? &proj_bottom : &proj_top);
    
	float xmin = ((drawOnBottomScreen ? 320 : 400) - W4_WIDTH) / 2.0f;
	float ymin = (240 - W4_HEIGHT) / 2.0f;
	float xmax = xmin + W4_WIDTH;
	float ymax = ymin + W4_HEIGHT;
	float txmax = ((float) W4_WIDTH / tex.width);
	float txmin = 0.0f;
	float tymin = ((float) W4_HEIGHT / tex.height);
	float tymax = 0.0f;

	C3D_TexBind(0, &tex);
	C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
		C3D_ImmSendAttrib(xmin, ymin, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmin, tymin, 0.0f, 0.0f);

		C3D_ImmSendAttrib(xmax, ymin, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmax, tymin, 0.0f, 0.0f);

		C3D_ImmSendAttrib(xmin, ymax, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmin, tymax, 0.0f, 0.0f);

		C3D_ImmSendAttrib(xmax, ymax, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmax, tymax, 0.0f, 0.0f);
	C3D_ImmDrawEnd();

	C3D_FrameEnd(0);
}

void platform_deinit(void) {
    // audio exit
#ifdef PLATFORM_HAS_APU
    platform_apu_deinit();
#endif

    // video exit
	linearFree(texBuf);

	C3D_TexDelete(&tex);

	C3D_RenderTargetDelete(target_top);
	C3D_RenderTargetDelete(target_bottom);

	C3D_Fini();
	gfxExit();

    // system exit
}