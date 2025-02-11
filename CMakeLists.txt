cmake_minimum_required(VERSION 3.22.1)
project(desktop)

set(CMAKE_CXX_STANDARD 20)

# ========Desktop configuration========
include_directories(platformio_nuli_avionics_flight_software/include)
set(desktopSimFiles platformio_nuli_avionics_flight_software/src/configurations/desktop.cpp
        platformio_nuli_avionics_flight_software/src/drivers/desktop/CustomCsvParser.cpp
        platformio_nuli_avionics_flight_software/src/core/AvionicsCore.cpp
        platformio_nuli_avionics_flight_software/src/core/Configuration.cpp
        platformio_nuli_avionics_flight_software/src/core/HardwareAbstraction.cpp
        platformio_nuli_avionics_flight_software/src/core/Logger.cpp
        platformio_nuli_avionics_flight_software/include/cli/Parser.cpp
        platformio_nuli_avionics_flight_software/include/cli/Parser.h
        platformio_nuli_avionics_flight_software/include/cli/Flag.cpp
        platformio_nuli_avionics_flight_software/include/cli/Flag.h
        platformio_nuli_avionics_flight_software/include/cli/Flag.tpp)

add_executable(desktop ${desktopSimFiles})



# ========Doxygen Build Configuration========
option(BUILD_DOC "Build documentation" ON)
find_package(Doxygen)
if (DOXYGEN_FOUND)
    add_custom_target(create_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs/
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)



# ========G-Test========
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# setting required files
set(cli_test
        platformio_nuli_avionics_flight_software/include/cli/Parser.cpp
        platformio_nuli_avionics_flight_software/include/cli/Parser.h
        platformio_nuli_avionics_flight_software/include/cli/Flag.cpp
        platformio_nuli_avionics_flight_software/include/cli/Flag.h
        platformio_nuli_avionics_flight_software/include/cli/Flag.tpp
)

# Enable Code Coverage Options
option(ENABLE_COVERAGE "Enable coverage reporting" ON)


# ----FlagTest----
add_executable(FlagTest ${cli_test}
        platformio_nuli_avionics_flight_software/include/cli/tests/FlagTest.cpp)

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
add_executable(ParserTest ${cli_test}
        platformio_nuli_avionics_flight_software/include/cli/tests/ParserTest.cpp)

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
