cmake_minimum_required(VERSION 3.27.1)
project(desktop)

set(CMAKE_CXX_STANDARD 20)

# ----------------------------------------------------------------------------
# Terminal-build note (CLion users can ignore this):
# If you build this project from a plain shell using CLion's bundled mingw
# toolchain (the usual default), prepend its bin/ directory to PATH first, e.g.
#   set "PATH=C:\Program Files\JetBrains\CLion <version>\bin\mingw\bin;%PATH%"
# Otherwise cc1plus.exe (the C++ frontend spawned by g++) cannot locate its
# co-installed DLLs (libgcc_s_seh-1.dll, libwinpthread-1.dll, libstdc++-6.dll,
# ...), exits with 0xC0000135 (STATUS_DLL_NOT_FOUND) and prints nothing, which
# makes the build appear to fail silently. CLion's own build action sets this
# PATH for you automatically.
# ----------------------------------------------------------------------------

include(FetchContent)

# ========Vendored libraries========
# All three live under platformio_nuli_avionics_flight_software/lib/ — see
# platformio_nuli_avionics_flight_software/lib/README for upstream/version/license info.
set(VENDOR_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/platformio_nuli_avionics_flight_software/lib)

# ETL (header-only) — replaces the prior FetchContent of etlcpp/etl. Exposed as
# the same `etl::etl` target name the FetchContent build provided, so existing
# target_link_libraries() lines keep working unchanged.
add_library(etl INTERFACE)
add_library(etl::etl ALIAS etl)
target_include_directories(etl INTERFACE "${VENDOR_LIB_DIR}/Embedded Template Library/include")

# Eigen (header-only) — used in source as `#include "Eigen/Dense"`. The Eigen
# source tree puts headers at lib/Eigen/Eigen/Dense (Eigen's own layout), so
# the include root has to be lib/Eigen/ for that include to resolve.
add_library(eigen INTERFACE)
add_library(Eigen3::Eigen ALIAS eigen)
target_include_directories(eigen INTERFACE ${VENDOR_LIB_DIR}/Eigen)

# Quaternion — small library with one .cpp; expose headers via INTERFACE target
# and add the source to consumers that need it (see desktopSimFiles below).
add_library(quaternion INTERFACE)
target_include_directories(quaternion INTERFACE ${VENDOR_LIB_DIR}/Quaternion)
set(QUATERNION_SOURCES ${VENDOR_LIB_DIR}/Quaternion/Quaternion.cpp)

# ========Desktop configuration========
include_directories(platformio_nuli_avionics_flight_software/include)
include_directories(platformio_nuli_avionics_flight_software/src)


# Collect all source files recursively in drivers/desktop and core
file(GLOB_RECURSE desktop_driver_sources
        platformio_nuli_avionics_flight_software/src/drivers/desktop/*.cpp
)

file(GLOB_RECURSE core_sources
        platformio_nuli_avionics_flight_software/src/core/*.cpp
        platformio_nuli_avionics_flight_software/src/core/*.tpp
)

set(desktopSimFiles
        platformio_nuli_avionics_flight_software/src/builds/desktop.cpp
        ${QUATERNION_SOURCES}
        ${desktop_driver_sources}
        ${core_sources}
)

# Create the desktop executable
add_executable(desktop ${desktopSimFiles})
target_link_libraries(desktop PRIVATE etl::etl Eigen3::Eigen quaternion)


# ========Doxygen Build Configuration========
option(BUILD_DOC "Build documentation" ON)
find_package(Doxygen)
find_program(DOT_EXECUTABLE dot)
if (BUILD_DOC AND DOXYGEN_FOUND AND DOT_EXECUTABLE)
    add_custom_target(create_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs/
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
else (BUILD_DOC)
    message("Doxygen need to be installed to generate the doxygen documentation")
endif ()



# ========G-Test========
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# setting required files
set(cliSimFiles
        platformio_nuli_avionics_flight_software/src/core/cli/Parser.cpp
        platformio_nuli_avionics_flight_software/src/core/cli/Parser.h
        platformio_nuli_avionics_flight_software/src/core/cli/Parser.tpp
        platformio_nuli_avionics_flight_software/src/core/cli/BaseFlag.cpp
        platformio_nuli_avionics_flight_software/src/core/cli/BaseFlag.h
        platformio_nuli_avionics_flight_software/src/core/cli/BaseFlag.tpp
        platformio_nuli_avionics_flight_software/src/core/cli/SimpleFlag.cpp
        platformio_nuli_avionics_flight_software/src/core/cli/SimpleFlag.h
        platformio_nuli_avionics_flight_software/src/core/cli/ArgumentFlag.h
        platformio_nuli_avionics_flight_software/src/core/cli/ArgumentFlag.tpp
        platformio_nuli_avionics_flight_software/src/core/cli/CLIEnums.h
        platformio_nuli_avionics_flight_software/src/core/cli/ReturnCodes.h
)

# Enable Code Coverage Options
option(ENABLE_COVERAGE "Enable coverage reporting" ON)


# ----FlagTest----
add_executable(FlagTest ${cliSimFiles}
        platformio_nuli_avionics_flight_software/include/tests/FlagTest.cpp)

target_link_libraries(
        FlagTest
        PRIVATE
        GTest::gtest_main
)

# Apply Coverage Flags to FlagTest1
if (ENABLE_COVERAGE)
    target_compile_options(FlagTest PRIVATE --coverage)  # Apply to actual target
    target_link_libraries(FlagTest PRIVATE --coverage)   # Apply to actual target
endif()


# ----ParserTest----
add_executable(ParserTest ${cliSimFiles}
        platformio_nuli_avionics_flight_software/include/tests/ParserTest.cpp)

target_link_libraries(
        ParserTest
        PRIVATE
        GTest::gtest_main
)

if (ENABLE_COVERAGE)
    target_compile_options(ParserTest PRIVATE --coverage)
    target_link_libraries(ParserTest PRIVATE --coverage)
endif()

# ++++Set G-Test++++
include(GoogleTest)
gtest_discover_tests(FlagTest)
gtest_discover_tests(ParserTest)

# ========cli example configuration========
add_executable(cli_example ${cliSimFiles}
        platformio_nuli_avionics_flight_software/include/CLIExample.cpp)