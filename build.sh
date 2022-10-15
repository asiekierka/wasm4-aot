#!/bin/bash

#while getopts opt; do
#
#done
#
#shift $((OPTIND - 1))

CARTRIDGE=$1
PLATFORM=$2
OUTPUT=$3
if [ -z "$OUTPUT" ]; then
	CARTRIDGE_BASENAME=`basename "$CARTRIDGE"`
	OUTPUT="${CARTRIDGE_BASENAME%.*}"
fi

# Build W2C2
cd w2c2
make
cd ..

# Convert cartridge to C
if [ ! -d cart ] ; then
	mkdir cart
fi
./w2c2/w2c2 -o cart/cart.c "$CARTRIDGE"

# Compile
case "$PLATFORM" in
gba)
	if [ -z "$DEVKITPRO" ]; then
		echo "Environment variable DEVKITPRO not set!"
		exit
	fi
	make -f Makefile.dkp-gba
	cp wasm4-aot-gba.gba "$OUTPUT".gba
	;;
nds)
	if [ -z "$DEVKITPRO" ]; then
		echo "Environment variable DEVKITPRO not set!"
		exit
	fi
	make -f Makefile.dkp-nds
	cp wasm4-aot-nds.nds "$OUTPUT".nds
	;;
3ds)
	if [ -z "$DEVKITPRO" ]; then
		echo "Environment variable DEVKITPRO not set!"
		exit
	fi
	make -f Makefile.dkp-3ds
	cp wasm4-aot-3ds.3dsx "$OUTPUT".3dsx
	;;
psp)
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
