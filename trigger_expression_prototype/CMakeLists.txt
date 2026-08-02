cmake_minimum_required(VERSION 3.20)
project(trigger_expression_prototype)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------------------------------------------------------
# Terminal-build note (CLion users can ignore this):
# If you build this project from a plain shell using CLion's bundled mingw
# toolchain (the usual default), prepend its bin/ directory to PATH first, e.g.
#   set "PATH=C:\Users\<you>\AppData\Local\Programs\CLion\bin\mingw\bin;%PATH%"
# Otherwise cc1plus.exe (the C++ frontend spawned by g++) cannot locate its
# co-installed DLLs (libgcc_s_seh-1.dll, libwinpthread-1.dll, libstdc++-6.dll,
# ...), exits with 0xC0000135 (STATUS_DLL_NOT_FOUND) and prints nothing, which
# makes the build appear to fail silently. CLion's own build action sets this
# PATH for you automatically.
# ----------------------------------------------------------------------------

include(FetchContent)

# This is a standalone sandbox: it reads (but never writes) the real firmware's struct
# definitions and vendored libs from ../platformio_nuli_avionics_flight_software, and has its own
# CMakeLists.txt/build dir so nothing here touches that project's build. A trigger condition
# written and tested here binds directly to the real RocketState_s, so porting it in later is just
# copying src/*.{h,cpp} over - no translation layer to rewrite.
set(FLIGHT_SOFTWARE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../platformio_nuli_avionics_flight_software)

add_library(quaternion_dep STATIC ${FLIGHT_SOFTWARE_DIR}/lib/Quaternion/Quaternion.cpp)
target_include_directories(quaternion_dep PUBLIC ${FLIGHT_SOFTWARE_DIR}/lib/Quaternion)

add_library(etl_dep INTERFACE)
target_include_directories(etl_dep INTERFACE "${FLIGHT_SOFTWARE_DIR}/lib/Embedded Template Library/include")

add_library(avionics_types INTERFACE)
target_include_directories(avionics_types INTERFACE ${FLIGHT_SOFTWARE_DIR}/src)
target_link_libraries(avionics_types INTERFACE quaternion_dep)

file(GLOB EXPRESSION_SOURCES src/*.cpp)

add_library(trigger_expressions ${EXPRESSION_SOURCES})
target_include_directories(trigger_expressions PUBLIC src)
target_link_libraries(trigger_expressions PUBLIC avionics_types etl_dep quaternion_dep)

# ========G-Test========
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(ExpressionTests tests/ExpressionTests.cpp)
target_link_libraries(ExpressionTests PRIVATE trigger_expressions GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(ExpressionTests)
