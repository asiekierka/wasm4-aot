/*
 * Copyright 2018 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wasm-rt-impl.h"

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if WASM_RT_MEMCHECK_SIGNAL_HANDLER_POSIX
#include <signal.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#else
// wasm4aot: remove
// #include <sys/mman.h>
#endif

#define PAGE_SIZE 65536
#define MAX_EXCEPTION_SIZE PAGE_SIZE

typedef struct FuncType {
  wasm_rt_type_t* params;
  wasm_rt_type_t* results;
  uint32_t param_count;
  uint32_t result_count;
} FuncType;

#if WASM_RT_MEMCHECK_SIGNAL_HANDLER
static bool g_signal_handler_installed = false;
static char* g_alt_stack;
#else
uint32_t wasm_rt_call_stack_depth;
uint32_t wasm_rt_saved_call_stack_depth;
#endif

static FuncType* g_func_types;
static uint32_t g_func_type_count;

jmp_buf wasm_rt_jmp_buf;

static uint32_t g_active_exception_tag;
static uint8_t g_active_exception[MAX_EXCEPTION_SIZE];
static uint32_t g_active_exception_size;

static jmp_buf* g_unwind_target;

void wasm_rt_trap(wasm_rt_trap_t code) {
  assert(code != WASM_RT_TRAP_NONE);
#if !WASM_RT_MEMCHECK_SIGNAL_HANDLER
  wasm_rt_call_stack_depth = wasm_rt_saved_call_stack_depth;
#endif
  WASM_RT_LONGJMP(wasm_rt_jmp_buf, code);
}

static bool func_types_are_equal(FuncType* a, FuncType* b) {
  if (a->param_count != b->param_count || a->result_count != b->result_count)
    return 0;
  uint32_t i;
  for (i = 0; i < a->param_count; ++i)
    if (a->params[i] != b->params[i])
      return 0;
  for (i = 0; i < a->result_count; ++i)
    if (a->results[i] != b->results[i])
      return 0;
  return 1;
}

uint32_t wasm_rt_register_func_type(uint32_t param_count,
                                    uint32_t result_count,
                                    ...) {
  FuncType func_type;
  func_type.param_count = param_count;
  func_type.params = malloc(param_count * sizeof(wasm_rt_type_t));
  func_type.result_count = result_count;
  func_type.results = malloc(result_count * sizeof(wasm_rt_type_t));

  va_list args;
  va_start(args, result_count);

  uint32_t i;
  // wasm4aot: fix va_arg compiler warning
  for (i = 0; i < param_count; ++i)
    func_type.params[i] = va_arg(args, int);
  for (i = 0; i < result_count; ++i)
    func_type.results[i] = va_arg(args, int);
  va_end(args);

  for (i = 0; i < g_func_type_count; ++i) {
    if (func_types_are_equal(&g_func_types[i], &func_type)) {
      free(func_type.params);
      free(func_type.results);
      return i + 1;
    }
  }

  uint32_t idx = g_func_type_count++;
  g_func_types = realloc(g_func_types, g_func_type_count * sizeof(FuncType));
  g_func_types[idx] = func_type;
  return idx + 1;
}

uint32_t wasm_rt_register_tag(uint32_t size) {
  static uint32_t s_tag_count = 0;

  if (size > MAX_EXCEPTION_SIZE) {
    wasm_rt_trap(WASM_RT_TRAP_EXHAUSTION);
  }
  return s_tag_count++;
}

void wasm_rt_load_exception(uint32_t tag, uint32_t size, const void* values) {
  assert(size <= MAX_EXCEPTION_SIZE);

  g_active_exception_tag = tag;
  g_active_exception_size = size;

  if (size) {
    memcpy(g_active_exception, values, size);
  }
}

WASM_RT_NO_RETURN void wasm_rt_throw(void) {
  WASM_RT_LONGJMP(*g_unwind_target, WASM_RT_TRAP_UNCAUGHT_EXCEPTION);
}

WASM_RT_UNWIND_TARGET* wasm_rt_get_unwind_target(void) {
  return g_unwind_target;
}

void wasm_rt_set_unwind_target(WASM_RT_UNWIND_TARGET* target) {
  g_unwind_target = target;
}

uint32_t wasm_rt_exception_tag(void) {
  return g_active_exception_tag;
}

uint32_t wasm_rt_exception_size(void) {
  return g_active_exception_size;
}

void* wasm_rt_exception(void) {
  return g_active_exception;
}

#if WASM_RT_MEMCHECK_SIGNAL_HANDLER_POSIX
static void signal_handler(int sig, siginfo_t* si, void* unused) {
  if (si->si_code == SEGV_ACCERR) {
    wasm_rt_trap(WASM_RT_TRAP_OOB);
  } else {
    wasm_rt_trap(WASM_RT_TRAP_EXHAUSTION);
  }
}
#endif

// wasm4aot: remove

void wasm_rt_init(void) {
#if WASM_RT_MEMCHECK_SIGNAL_HANDLER_POSIX
  if (!g_signal_handler_installed) {
    g_signal_handler_installed = true;

    /* Use alt stack to handle SIGSEGV from stack overflow */
    g_alt_stack = malloc(SIGSTKSZ);
    if (g_alt_stack == NULL) {
      perror("malloc failed");
      abort();
    }

    stack_t ss;
    ss.ss_sp = g_alt_stack;
    ss.ss_flags = 0;
    ss.ss_size = SIGSTKSZ;
    if (sigaltstack(&ss, NULL) != 0) {
      perror("sigaltstack failed");
      abort();
    }

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = signal_handler;

    /* Install SIGSEGV and SIGBUS handlers, since macOS seems to use SIGBUS. */
    if (sigaction(SIGSEGV, &sa, NULL) != 0 ||
        sigaction(SIGBUS, &sa, NULL) != 0) {
      perror("sigaction failed");
      abort();
    }
  }
#endif
}

bool wasm_rt_is_initialized(void) {
#if WASM_RT_MEMCHECK_SIGNAL_HANDLER_POSIX
  return g_signal_handler_installed;
#else
  return true;
#endif
}

void wasm_rt_free(void) {
#if WASM_RT_MEMCHECK_SIGNAL_HANDLER_POSIX
  free(g_alt_stack);
#endif
}

void wasm_rt_allocate_memory(wasm_rt_memory_t* memory,
                             uint32_t initial_pages,
                             uint32_t max_pages) {
  // wasm4aot: remove
}

uint32_t wasm_rt_grow_memory(wasm_rt_memory_t* memory, uint32_t delta) {
  // wasm4aot: remove
  return (uint32_t)-1;
}

void wasm_rt_free_memory(wasm_rt_memory_t* memory) {
  // wasm4aot: remove
}

void wasm_rt_allocate_table(wasm_rt_table_t* table,
                            uint32_t elements,
                            uint32_t max_elements) {
  table->size = elements;
  table->max_size = max_elements;
  table->data = calloc(table->size, sizeof(wasm_rt_elem_t));
}

void wasm_rt_free_table(wasm_rt_table_t* table) {
  free(table->data);
}

const char* wasm_rt_strerror(wasm_rt_trap_t trap) {
  switch (trap) {
    case WASM_RT_TRAP_NONE:
      return "No error";
    case WASM_RT_TRAP_OOB:
#if WASM_RT_MERGED_OOB_AND_EXHAUSTION_TRAPS
      return "Out-of-bounds access in linear memory or call stack exhausted";
#else
      return "Out-of-bounds access in linear memory";
    case WASM_RT_TRAP_EXHAUSTION:
      return "Call stack exhausted";
#endif
    case WASM_RT_TRAP_INT_OVERFLOW:
      return "Integer overflow on divide or truncation";
    case WASM_RT_TRAP_DIV_BY_ZERO:
      return "Integer divide by zero";
    case WASM_RT_TRAP_INVALID_CONVERSION:
      return "Conversion from NaN to integer";
    case WASM_RT_TRAP_UNREACHABLE:
      return "Unreachable instruction executed";
    case WASM_RT_TRAP_CALL_INDIRECT:
      return "Invalid call_indirect";
    case WASM_RT_TRAP_UNCAUGHT_EXCEPTION:
      return "Uncaught exception";
  }
  return "invalid trap code";
}
