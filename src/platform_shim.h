#pragma once

#include "build_config.h"
#include "platform.h"

#ifndef PLATFORM_HAS_DRAW_PARTIAL
#define platform_draw_partial(x, y, width, height)
#endif
#ifndef debug_printf
#define debug_printf(...)
#endif