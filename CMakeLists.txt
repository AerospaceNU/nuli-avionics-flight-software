cmake_minimum_required(VERSION 3.27.1)
project(desktop)

set(CMAKE_CXX_STANDARD 20)

include_directories(platformio_nuli_avionics_flight_software/include)
set(desktopSimFiles platformio_nuli_avionics_flight_software/src/configurations/desktop.cpp
        platformio_nuli_avionics_flight_software/src/drivers/desktop/CustomCsvParser.cpp)

add_executable(desktop ${desktopSimFiles})



# Doxygen Build Configuration
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