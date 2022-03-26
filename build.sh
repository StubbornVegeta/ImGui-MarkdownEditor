#!/bin/sh
mkdir build
cd build
cmake ..
cp ../FiraCode-Bold.ttf ./
cp ../imgui.ini ./
make
