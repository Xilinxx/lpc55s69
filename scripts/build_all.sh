#!/bin/bash

./scripts/build_lpcxpresso.sh
make -C build_lpcexpresso

./scripts/build_zeus300s.sh
make -C build_zeus
