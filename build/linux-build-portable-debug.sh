#!/bin/bash

type cmake >/dev/null 2>&1 || { echo >&2 "Can't find cmake."; exit 1; }

BUILD_DIR="`uname`-`uname -m`-debug"
mkdir "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -G "Unix Makefiles" -DUseInternalPNG=OFF -DUseInternalJPEG=OFF \
	  -DUseInternalZLIB=OFF -DCMake_Build_Type=Debug ../..

make
