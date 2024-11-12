#!/bin/bash
./build.sh
gdb -x gdbinit ./build/bin/portamento
