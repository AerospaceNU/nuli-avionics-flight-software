#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H

#include <cstdint>
#include <array>

/**
 * @file Avionics
 * @brief Global definitions for the codebase
 */

#define remove_padding __attribute__((packed))

//using std::array;

const uint8_t MAX_PYRO_NUM = 10;
const uint8_t MAX_BAROMETER_NUM = 4;
const uint8_t MAX_ACCELEROMETER_NUM = 4;
const uint8_t MAX_MAGNETOMETER_NUM = 4;
const uint8_t MAX_GYROSCOPE_NUM = 4;
const uint8_t MAX_GPS_NUM = 4;
const uint8_t MAX_FLASH_MEMORY_NUM = 4;
const uint8_t MAX_COMMUNICATION_LINK_NUM = 4;


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H
