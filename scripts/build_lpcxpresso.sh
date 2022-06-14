#!/bin/bash

mkdir -p build_lpcexpresso
cd build_lpcexpresso && cmake -DCMAKE_TOOLCHAIN_FILE=./cmake/toolchain/arm-none-eabi.cmake -DBOARD_NAME="lpcxpresso55s69" ../
