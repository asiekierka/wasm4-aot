project('wasm4-aot', ['c', 'cpp'])

dep_sdl2 = dependency('sdl2')

executable('wasm4-aot', [
	'../src/framebuffer.cpp',
	'../src/main.c',
	'../src/runtime.c',
	'../src/util.c',
	'../src/wasm-shim.c',
	'../src/apu/apu.c',
	'../src/platform/pc-sdl2/platform.c',
	'../cart/cart.c'
],
	c_args: '-fstack-protector-all',
	dependencies: [dep_sdl2],
	include_directories : include_directories('../cart', '../config', '../src', '../src/apu', '../src/platform/pc-sdl2', '../src/wasm2c')
)
