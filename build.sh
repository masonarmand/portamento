#!/bin/bash
# check if build directory exists
if [ ! -d "build" ]; then
  mkdir build
fi
cd build
cmake ..

if [[ "$1" == "--install" ]]; then
        sudo make install
else
        make
fi
