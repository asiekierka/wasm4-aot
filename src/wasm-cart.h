#pragma once

#include "build_config.h"

#if defined(BUILD_USE_WASM2C)

#elif defined(BUILD_USE_W2C2)
#include "w2c2_base.h"

extern void (*e_X5Fstart)();
extern void (*e_X5Finitialize)();
extern void (*e_start)();
extern void (*e_update)();
extern void init(void);
#else
#error Please define a WASM transpiler!
#endif