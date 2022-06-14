#!/bin/bash

mkdir -p build_zeus
cd build_zeus && cmake -DCMAKE_TOOLCHAIN_FILE=./cmake/toolchain/arm-none-eabi.cmake -DBOARD_NAME="zeus300s" ../
