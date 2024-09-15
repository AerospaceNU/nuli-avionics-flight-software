cmake_minimum_required(VERSION 3.29)
project(desktop)

set(CMAKE_CXX_STANDARD 20)

add_executable(desktop platformio_nuli_avionics_flight_software/src/desktop.cpp)
