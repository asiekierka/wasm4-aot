#include <math.h>
#include "wasm-rt.h"

float wasm_rt_fabs(float x) {
    return fabs(x);
}

float wasm_rt_fabsf(float x) {
    return fabsf(x);
}