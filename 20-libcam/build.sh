#!/bin/bash
set -e
LIBCAM=/home/abdu/.local/libcamera-install
cd "$(dirname "$0")"

g++ -std=c++17 -I"$LIBCAM/include/libcamera" main.cpp -o main \
	-L"$LIBCAM/lib/x86_64-linux-gnu" -lcamera -lcamera-base \
	-Wl,-rpath,"$LIBCAM/lib/x86_64-linux-gnu" -Wl,--disable-new-dtags
