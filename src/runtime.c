#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "framebuffer.h"
#include "platform_shim.h"
#include "runtime.h"
#include "util.h"
#include "wasm-cart.h"

#ifdef PLATFORM_HAS_APU
#include "apu.h"
#endif

// TODO: SRAM handling
w4_Disk w4_disk;

#define memory w4_memory
#define disk w4_disk

void w4_runtimeInit(void) {
    // Set memory to initial state
    memset(&memory, 0, 1 << 16);
    w4_write32LE(&w4_memory.palette[0], 0xe0f8cf);
    w4_write32LE(&w4_memory.palette[1], 0x86c06c);
    w4_write32LE(&w4_memory.palette[2], 0x306850);
    w4_write32LE(&w4_memory.palette[3], 0x071821);
    w4_memory.drawColors[0] = 0x03;
    w4_memory.drawColors[1] = 0x12;
    w4_write16LE(&w4_memory.mouseX, 0x7fff);
    w4_write16LE(&w4_memory.mouseY, 0x7fff);

#ifdef PLATFORM_HAS_APU
    w4_apuInit();
#endif
}

void w4_runtimeUpdate (void) {
    if (!(memory.systemFlags & W4_SYSTEM_PRESERVE_FRAMEBUFFER)) {
        w4_framebufferClear();
    }
    e_update();
}

void w4_runtimeSetGamepad (int idx, uint8_t gamepad) {
    memory.gamepads[idx] = gamepad;
}

void w4_runtimeSetMouse (int16_t x, int16_t y, uint8_t buttons) {
    w4_write16LE(&memory.mouseX, x);
    w4_write16LE(&memory.mouseY, y);
    memory.mouseButtons = buttons;
}

void w4_runtimeBlit (const uint8_t* sprite, int x, int y, int width, int height, int flags) {
    // debug_printf("blit: %p, %d, %d, %d, %d, %d\n", sprite, x, y, width, height, flags);

    w4_runtimeBlitSub(sprite, x, y, width, height, 0, 0, width, flags);
}

void w4_runtimeBlitSub (const uint8_t* sprite, int x, int y, int width, int height, int srcX, int srcY, int stride, int flags) {
    // debug_printf("blitSub: %p, %d, %d, %d, %d, %d, %d, %d, %d\n", sprite, x, y, width, height, srcX, srcY, stride, flags);

    bool bpp2 = (flags & 1);
    bool flipX = (flags & 2);
    bool flipY = (flags & 4);
    bool rotate = (flags & 8);
    w4_framebufferBlit(sprite, x, y, width, height, srcX, srcY, stride, bpp2, flipX, flipY, rotate);
}

void w4_runtimeLine (int x1, int y1, int x2, int y2) {
    // debug_printf("line: %d, %d, %d, %d\n", x1, y1, x2, y2);
    w4_framebufferLine(x1, y1, x2, y2);
}

void w4_runtimeHLine (int x, int y, int len) {
    // debug_printf("hline: %d, %d, %d\n", x, y, len);
    w4_framebufferHLine(x, y, len);
}

void w4_runtimeVLine (int x, int y, int len) {
    // debug_printf("vline: %d, %d, %d\n", x, y, len);
    w4_framebufferVLine(x, y, len);
}

void w4_runtimeOval (int x, int y, int width, int height) {
    // debug_printf("oval: %d, %d, %d, %d\n", x, y, width, height);
    w4_framebufferOval(x, y, width, height);
}

void w4_runtimeRect (int x, int y, int width, int height) {
    // debug_printf("rect: %d, %d, %d, %d\n", x, y, width, height);
    w4_framebufferRect(x, y, width, height);
}

void w4_runtimeText (const uint8_t* str, int x, int y) {
    // debug_printf("text: %s, %d, %d\n", str, x, y);
    w4_framebufferText(str, x, y);
}

void w4_runtimeTextUtf8 (const uint8_t* str, int byteLength, int x, int y) {
    // debug_printf("textUtf8: %p, %d, %d, %d\n", str, byteLength, x, y);
    w4_framebufferTextUtf8(str, byteLength, x, y);
}

void w4_runtimeTextUtf16 (const uint16_t* str, int byteLength, int x, int y) {
    // debug_printf("textUtf16: %p, %d, %d, %d\n", str, byteLength, x, y);
    w4_framebufferTextUtf16(str, byteLength, x, y);
}

void w4_runtimeTone (int frequency, int duration, int volume, int flags) {
    debug_printf("tone: %d, %d, %d, %d\n", frequency, duration, volume, flags);
#ifdef PLATFORM_HAS_APU
    w4_apuTone(frequency, duration, volume, flags);
#endif
}

int w4_runtimeDiskr (uint8_t* dest, int size) {
#ifdef PLATFORM_HAS_DISK
    if (size > disk.size) {
        size = disk.size;
    }
    memcpy(dest, disk.data, size);
    return size;
#else
    return 0;
#endif
}

int w4_runtimeDiskw (const uint8_t* src, int size) {
#ifdef PLATFORM_HAS_DISK
    if (size > 1024) {
        size = 1024;
    }
    disk.size = size;
    memcpy(disk.data, src, size);
    return size;
#else
    return 0;
#endif
}

void w4_runtimeTrace (const uint8_t* str) {
    debug_printf("%s\n", str);
}

void w4_runtimeTraceUtf8 (const uint8_t* str, int byteLength) {
    debug_printf("%.*s\n", byteLength, str);
}

void w4_runtimeTraceUtf16 (const uint16_t* str, int byteLength) {
    debug_printf("TODO: traceUtf16: %p, %d\n", str, byteLength);
}

void w4_runtimeTracef (const uint8_t* str, const void* stack) {
    debug_printf("%s\n", str);

    // This seems to crash on Linux release builds
    // vdebug_printf(str, (void*)&stack);
    // putchar('\n');
}
