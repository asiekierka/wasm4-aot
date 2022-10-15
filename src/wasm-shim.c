#include <stdint.h>
#include "runtime.h"
#include "stdlib.h"
#include "wasm-cart.h"

typedef uint32_t u32;

static wasmMemory wasm_shim_memory = {
    (uint8_t*) &w4_memory, 1, 1, 65536
};

wasmMemory *m_env_memory = &wasm_shim_memory;
uint8_t *w4_memory_raw = (uint8_t*) &w4_memory;

#define AS_NATIVE_PTR(x) (((uint8_t*) &w4_memory) + (x))

void trap(Trap) {
    abort();
}

void e_dummy(void) {

}

// Some WASM-4 cartridges don't come with e_start.
void (*e_start)(void) __attribute__((weak)) = e_dummy;

// void w4_runtimeBlit (const uint8_t* sprite, int x, int y, int width, int height, int flags);
void wasm_shim_blit(u32 sprite, u32 x, u32 y, u32 width, u32 height, u32 flags) {
    w4_runtimeBlit(AS_NATIVE_PTR(sprite), x, y, width, height, flags);
}
void (*f_env_blit)(u32, u32, u32, u32, u32, u32) = wasm_shim_blit;

// void w4_runtimeBlitSub (const uint8_t* sprite, int x, int y, int width, int height, int srcX, int srcY, int stride, int flags);
void wasm_shim_blitSub(u32 sprite, u32 x, u32 y, u32 width, u32 height, u32 srcX, u32 srcY, u32 stride, u32 flags) {
    w4_runtimeBlitSub(AS_NATIVE_PTR(sprite), x, y, width, height, srcX, srcY, stride, flags);
}
void (*f_env_blitSub)(u32, u32, u32, u32, u32, u32, u32, u32, u32) = wasm_shim_blitSub;

// void w4_runtimeLine (int x1, int y1, int x2, int y2);
void wasm_shim_line(u32 x1, u32 y1, u32 x2, u32 y2) {
    w4_runtimeLine(x1, y1, x2, y2);
}
void (*f_env_line)(u32, u32, u32, u32) = wasm_shim_line;

// void w4_runtimeHLine (int x, int y, int len);
void wasm_shim_hline(u32 x, u32 y, u32 len) {
    w4_runtimeHLine(x, y, len);
}
void (*f_env_hline)(u32, u32, u32) = wasm_shim_hline;

// void w4_runtimeVLine (int x, int y, int len);
void wasm_shim_vline(u32 x, u32 y, u32 len) {
    w4_runtimeVLine(x, y, len);
}
void (*f_env_vline)(u32, u32, u32) = wasm_shim_vline;

// void w4_runtimeOval (int x, int y, int width, int height);
void wasm_shim_oval(u32 x, u32 y, u32 width, u32 height) {
    w4_runtimeOval(x, y, width, height);
}
void (*f_env_oval)(u32, u32, u32, u32) = wasm_shim_oval;

// void w4_runtimeRect (int x, int y, int width, int height);
void wasm_shim_rect(u32 x, u32 y, u32 width, u32 height) {
    w4_runtimeRect(x, y, width, height);
}
void (*f_env_rect)(u32, u32, u32, u32) = wasm_shim_rect;

// void w4_runtimeText (const uint8_t* str, int x, int y);
void wasm_shim_text(u32 str, u32 x, u32 y) {
    w4_runtimeText(AS_NATIVE_PTR(str), x, y);
}
void (*f_env_text)(u32, u32, u32) = wasm_shim_text;

// void w4_runtimeTextUtf8 (const uint8_t* str, int byteLength, int x, int y);
void wasm_shim_textUtf8(u32 str, u32 byteLength, u32 x, u32 y) {
    w4_runtimeTextUtf8(AS_NATIVE_PTR(str), byteLength, x, y);
}
void (*f_env_textUtf8)(u32, u32, u32, u32) = wasm_shim_textUtf8;

// void w4_runtimeTextUtf16 (const uint16_t* str, int byteLength, int x, int y);
void wasm_shim_textUtf16(u32 str, u32 byteLength, u32 x, u32 y) {
    w4_runtimeTextUtf16((uint16_t*) AS_NATIVE_PTR(str), byteLength, x, y);
}
void (*f_env_textUtf16)(u32, u32, u32, u32) = wasm_shim_textUtf16;

// void w4_runtimeTone (int frequency, int duration, int volume, int flags);
void wasm_shim_tone(u32 frequency, u32 duration, u32 volume, u32 flags) {
    w4_runtimeTone(frequency, duration, volume, flags);
}
void (*f_env_tone)(u32, u32, u32, u32) = wasm_shim_tone;

// int w4_runtimeDiskr (uint8_t* dest, int size);
u32 wasm_shim_diskr(u32 ptr, u32 size) {
    return w4_runtimeDiskr(AS_NATIVE_PTR(ptr), size);
}
u32 (*f_env_diskr)(u32, u32) = wasm_shim_diskr;

// int w4_runtimeDiskw (const uint8_t* src, int size);
u32 wasm_shim_diskw(u32 ptr, u32 size) {
    return w4_runtimeDiskw(AS_NATIVE_PTR(ptr), size);
}
u32 (*f_env_diskw)(u32, u32) = wasm_shim_diskw;

// void w4_runtimeTrace (const uint8_t* str);
void wasm_shim_trace(u32 str) {
    w4_runtimeTrace(AS_NATIVE_PTR(str));
}
void (*f_env_trace)(u32) = wasm_shim_trace;

// void w4_runtimeTraceUtf8 (const uint8_t* str, int byteLength);
void wasm_shim_traceUtf8(u32 str, u32 byteLength) {
    w4_runtimeTraceUtf8(AS_NATIVE_PTR(str), byteLength);
}
void (*f_env_traceUtf8)(u32, u32) = wasm_shim_traceUtf8;

// void w4_runtimeTraceUtf16 (const uint16_t* str, int byteLength);
void wasm_shim_traceUtf16(u32 str, u32 byteLength) {
    w4_runtimeTraceUtf16((uint16_t*) AS_NATIVE_PTR(str), byteLength);
}
void (*f_env_traceUtf16)(u32, u32) = wasm_shim_traceUtf16;

void wasm_shim_tracef(u32 a, u32 b) {
    // TODO
}
void (*f_env_tracef)(u32, u32) = wasm_shim_tracef;