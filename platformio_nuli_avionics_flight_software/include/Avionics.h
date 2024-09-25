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

const double ATMOSPHERIC_PRESSURE_PA = 101325;
const double GRAVITATIONAL_ACCELERATION_M_SS = 9.80665;
const double LAPSE_RATE_K_M = 0.0065;
const double GAS_CONSTANT_J_KG_K = 287.0474909;

const uint8_t MAX_PYRO_NUM = 10;
const uint8_t MAX_BAROMETER_NUM = 4;
const uint8_t MAX_ACCELEROMETER_NUM = 4;
const uint8_t MAX_MAGNETOMETER_NUM = 4;
const uint8_t MAX_GYROSCOPE_NUM = 4;
const uint8_t MAX_GPS_NUM = 4;
const uint8_t MAX_FLASH_MEMORY_NUM = 4;
const uint8_t MAX_COMMUNICATION_LINK_NUM = 4;


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H
