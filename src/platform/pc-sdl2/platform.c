#include <stdio.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include "build_config.h"
#include "platform.h"
#include "runtime.h"

w4_Memory w4_memory;

static SDL_Window *window;
static SDL_Surface *surface;
static uint8_t held_keys;
static uint32_t last_draw = 0;

void platform_init(void) {
    held_keys = 0;

    // TODO: error handling etc
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("wasm4 aot",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160, 160, 0);
}

bool platform_update(void) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                uint8_t mask = 0;
                if (event.key.keysym.sym == SDLK_z) {
                    mask = W4_BUTTON_Z;
                } else if (event.key.keysym.sym == SDLK_x) {
                    mask = W4_BUTTON_X;
                } else if (event.key.keysym.sym == SDLK_UP) {
                    mask = W4_BUTTON_UP;
                } else if (event.key.keysym.sym == SDLK_LEFT) {
                    mask = W4_BUTTON_LEFT;
                } else if (event.key.keysym.sym == SDLK_RIGHT) {
                    mask = W4_BUTTON_RIGHT;
                } else if (event.key.keysym.sym == SDLK_DOWN) {
                    mask = W4_BUTTON_DOWN;
                }
                if (event.type == SDL_KEYDOWN) {
                    held_keys |= mask;
                } else {
                    held_keys &= ~mask;
                }
                w4_runtimeSetGamepad(0, held_keys);
            } break;
            case SDL_QUIT:
                return false;
        }
    }

    return true;
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

    int32_t diff = (last_draw + 16) - SDL_GetTicks();

    if (diff > 0) {
        SDL_Delay(diff);
    }

    last_draw = SDL_GetTicks();
}

void platform_deinit(void) {
    // TODO
}
