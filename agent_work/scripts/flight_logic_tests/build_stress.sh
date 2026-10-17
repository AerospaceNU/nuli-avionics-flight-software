#!/bin/bash
# Build stress_test.exe against the firmware's core sources
cd "$(dirname "$0")"
export PATH="${HOST_GXX_DIR:-/c/Users/patri/AppData/Local/Programs/CLion/bin/mingw/bin}:$PATH"
FW=../../../platformio_nuli_avionics_flight_software
g++ -std=gnu++17 -O2 -w -I $FW/src -I $FW/include -I $FW/lib/Quaternion -I $FW/lib/Eigen -I "$FW/lib/Embedded Template Library/include" ${SRC:-stress_test.cpp} $(find $FW/src/core -name '*.cpp') $FW/lib/Quaternion/Quaternion.cpp -o ${1:-stress_test.exe}
