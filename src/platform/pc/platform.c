#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include "runtime.h"

w4_Memory w4_memory;

static SDL_Window *window;
static SDL_Surface *surface;

void platform_init(void) {
    // TODO: error handling etc
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("wasm4 aot",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160, 160, 0);
}

void platform_update(void) {
    // TODO
}

#define TO_WINDOW_SURFACE(x) ((x) | 0xFF000000)

void platform_draw(void) {
    surface = SDL_GetWindowSurface(window);
    uint32_t *palette = w4_memory.palette;

    SDL_LockSurface(surface);
    int n = 0;
    for (int y = 0; y < 160; y++) {
        uint32_t *out = surface->pixels + (surface->pitch * y);
        for (int x = 0; x < 160; x+=4, n++) {
            uint8_t quartet = w4_memory.framebuffer[n];
            int color1 = (quartet & 0b00000011) >> 0;
            int color2 = (quartet & 0b00001100) >> 2;
            int color3 = (quartet & 0b00110000) >> 4;
            int color4 = (quartet & 0b11000000) >> 6;

            *out++ = TO_WINDOW_SURFACE(palette[color1]);
            *out++ = TO_WINDOW_SURFACE(palette[color2]);
            *out++ = TO_WINDOW_SURFACE(palette[color3]);
            *out++ = TO_WINDOW_SURFACE(palette[color4]);
        }
    }
    SDL_UnlockSurface(surface);

    SDL_UpdateWindowSurface(window);

    SDL_Delay(10); // TODO
}

void platform_deinit(void) {
    // TODO
}
