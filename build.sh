#!/bin/sh
mkdir build
cd build
cmake ..
cp ../imgui.ini ./
make
