#!/bin/bash

usage() {
	echo "Usage: $0 [options] cartridge.wasm platform [output_basename]" 1>&2
	echo "" 1>&2
	echo "  -a x   Set author name" 1>&2
	echo "  -d     Enable debug output (nds, 3ds)" 1>&2
	echo "  -f x   Select C transpiler: wasm2c, w2c2 (default)" 1>&2
	echo "  -n x   Set game name" 1>&2
	exit 1
}

DEBUG=
FRONTEND=w2c2
GAME_AUTHOR=-
GAME_NAME=

while getopts ":a:df:n:" opt; do
	case "${opt}" in
		a)
			GAME_AUTHOR="${OPTARG}"
			;;
		d)
			DEBUG=true
			;;
		f)
			FRONTEND="${OPTARG}"
			((s == "wasm2c" || s == "w2c2")) || usage
			;;
		n)
			GAME_NAME="${OPTARG}"
			;;
		*)
			usage
			;;
	esac
done

shift $((OPTIND - 1))

CARTRIDGE=$1
PLATFORM=$2
OUTPUT=$3

if [ -z "$CARTRIDGE" ] || [ -z "$PLATFORM" ]; then
	usage
fi

if [ -z "$OUTPUT" ]; then
	CARTRIDGE_BASENAME=`basename "$CARTRIDGE"`
	OUTPUT="${CARTRIDGE_BASENAME%.*}"
fi

if [ -z "$GAME_NAME" ]; then
	GAME_NAME="$OUTPUT"
fi

# Generate build_config.h
if [ ! -d config ] ; then
	mkdir config
fi

echo "#pragma once" > config/build_config.h
echo "" >> config/build_config.h
case "$FRONTEND" in
wasm2c)
	echo "#define BUILD_USE_WASM2C 1" >> config/build_config.h
	;;
w2c2)
	echo "#define BUILD_USE_W2C2 1" >> config/build_config.h
	;;
esac
echo "#define BUILD_GAME_NAME \""$GAME_NAME"\"" >> config/build_config.h
echo "#define BUILD_GAME_AUTHOR \""$GAME_AUTHOR"\"" >> config/build_config.h
if [ ! -z "$DEBUG" ]; then
	echo "#define DEBUG 1" >> config/build_config.h
fi

# Convert cartridge to C
if [ ! -d cart ] ; then
	mkdir cart
fi

case "$FRONTEND" in
wasm2c)
	wasm2c -o cart/cart.c -n cart "$CARTRIDGE"
	;;
w2c2)
	cd w2c2
	make
	cd ..
	./w2c2/w2c2 -o cart/cart.c "$CARTRIDGE"
	;;
esac

# Compile
case "$PLATFORM" in
gba)
	if [ -z "$DEVKITPRO" ]; then
		echo "Environment variable DEVKITPRO not set!"
		exit
	fi
	GAME_NAME="$GAME_NAME" GAME_AUTHOR="$GAME_AUTHOR" \
	make -f Makefile.dkp-gba
	cp wasm4-aot-gba.gba "$OUTPUT".gba
	;;
nds)
	if [ -z "$DEVKITPRO" ]; then
		echo "Environment variable DEVKITPRO not set!"
		exit
	fi
	GAME_NAME="$GAME_NAME" GAME_AUTHOR="$GAME_AUTHOR" \
	make -f Makefile.dkp-nds
	cp wasm4-aot-nds.nds "$OUTPUT".nds
	;;
3ds)
	if [ -z "$DEVKITPRO" ]; then
		echo "Environment variable DEVKITPRO not set!"
		exit
	fi
	GAME_NAME="$GAME_NAME" GAME_AUTHOR="$GAME_AUTHOR" \
	make -f Makefile.dkp-3ds
	cp wasm4-aot-3ds.3dsx "$OUTPUT".3dsx
	;;
psp)
	if [ -f build-psp/PARAM.SFO ]; then
		rm build-psp/PARAM.SFO
	fi
	GAME_NAME="$GAME_NAME" GAME_AUTHOR="$GAME_AUTHOR" \
	make -f Makefile.psp
	if [ -d build-psp/GAME ]; then
		rm -r build-psp/GAME
	fi
	mkdir -p build-psp/GAME/"$OUTPUT"/
	cp build-psp/EBOOT.PBP build-psp/GAME/"$OUTPUT"/
	cd build-psp
	zip -9 -r ../"$OUTPUT"-psp.zip GAME
	cd ..
	;;
*)
	echo "Unknown platform "$PLATFORM"!"
	exit
	;;
esac
