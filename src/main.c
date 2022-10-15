#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform_shim.h"
#include "runtime.h"
#include "wasm-cart.h"

int main(int argc, char **argv) {
    platform_init();

    w4_runtimeInit();

    init();

    e_start();

    while (platform_update()) {
        w4_runtimeUpdate();
        platform_draw();
    }

    platform_deinit();

    return 0;
}
