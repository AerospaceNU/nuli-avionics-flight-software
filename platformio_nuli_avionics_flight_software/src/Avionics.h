#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H

#include <cstdint>
#include <cmath>

/**
 * @file Avionics
 * @brief Global definitions for the codebase
 */

#define remove_struct_padding __attribute__((packed))

const uint8_t MAX_PYRO_NUM = 10;
const uint8_t MAX_VOLTAGE_SENSOR_NUM = 10;
const uint8_t MAX_BAROMETER_NUM = 4;
const uint8_t MAX_ACCELEROMETER_NUM = 4;
const uint8_t MAX_MAGNETOMETER_NUM = 4;
const uint8_t MAX_GYROSCOPE_NUM = 4;
const uint8_t MAX_GPS_NUM = 4;
const uint8_t MAX_FLASH_MEMORY_NUM = 4;
const uint8_t MAX_FRAM_MEMORY_NUM = 4;
const uint8_t MAX_RADIO_TRANSMITTER_LINK_NUM = 4;
const uint8_t MAX_INDICATOR_NUM = 4;
const uint8_t MAX_GENERIC_SENSOR_NUM = 10;

const uint8_t MAX_CONFIGURATION_NUM = 30;
const uint8_t MAX_CONFIGURATION_LENGTH = 50;

/**
 * @struct Vector3D_s
 * @brief Vector with 3 axis
 */
struct Vector3D_s {
    double x;           ///< X axis
    double y;           ///< Y axis
    double z;           ///< Z axis
};






#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H
