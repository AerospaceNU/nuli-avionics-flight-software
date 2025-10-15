#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H

#include <cstdint>
#include <cmath>

/**
 * @file Avionics.h
 * @brief Global definitions for the codebase
 */

// Define state transition paramiters
constexpr float LAUNCH_ACCELERATION_THRESHOLD_MSS = 20.0;
constexpr float LAUNCH_ALTITUDE_THRESHOLD_M = 100.0;
constexpr uint32_t LAUNCH_DEBOUNCE_TIMER_MS = 150;
constexpr uint32_t APOGEE_DEBOUNCE_TIMER_MS = 150;
constexpr float APOGEE_ALTITUDE_CHANGE_THRESHOLD_M = 2.0;
constexpr float LANDING_ALTITUDE_CHANGE_THRESHOLD_M = 3.0;
constexpr uint32_t LANDING_DEBOUNCE_TIMER_MS = 3000;
constexpr uint32_t UNKNOWN_STATE_TIMER_MS = 1000;
constexpr float UNKNOWN_STATE_ALTITUDE_CHANGE_THRESHOLD_M = 5.0;
constexpr float UNKNOWN_STATE_VELOCITY_THRESHOLD_MS = 3.0;


// Hardware Abstraction max size paramiters
constexpr uint8_t MAX_PYRO_NUM = 10;
constexpr uint8_t MAX_VOLTAGE_SENSOR_NUM = 10;
constexpr uint8_t MAX_BAROMETER_NUM = 4;
constexpr uint8_t MAX_ACCELEROMETER_NUM = 4;
constexpr uint8_t MAX_MAGNETOMETER_NUM = 4;
constexpr uint8_t MAX_GYROSCOPE_NUM = 4;
constexpr uint8_t MAX_GPS_NUM = 4;
constexpr uint8_t MAX_FLASH_MEMORY_NUM = 4;
constexpr uint8_t MAX_FRAM_MEMORY_NUM = 4;
constexpr uint8_t MAX_RADIO_TRANSMITTER_LINK_NUM = 4;
constexpr uint8_t MAX_INDICATOR_NUM = 4;
constexpr uint8_t MAX_GENERIC_SENSOR_NUM = 10;
// Configuration max size paramiters
constexpr uint8_t MAX_CONFIGURATION_NUM = 30;
constexpr uint8_t MAX_CONFIGURATION_LENGTH = 120;


/**
 * @struct Vector3D_s
 * @brief Vector with 3 axis
 */
struct Vector3D_s {
    float x; ///< X axis
    float y; ///< Y axis
    float z; ///< Z axis
};

/**
 * @struct Coordinates_s
 * @brief Geographic coordinates with latitude and longitude
 * @details Intended for use with GPS
 */
struct Coordinates_s {
    float latitudeDeg; ///< Latitude in degrees
    float longitudeDeg; ///< Longitude in degrees
    float altitudeM; ///< Altitude in meters
};

struct Timestamp_s {
    uint32_t runtime_ms;
    uint32_t dt_ms;
    uint32_t tick;
};

enum FlightState_e {
    PRE_FLIGHT = 0,
    ASCENT = 1,
    DESCENT = 2,
    POST_FLIGHT = 3,
    UNKNOWN_FLIGHT_STATE = 4,
};

struct State1D_s {
    float altitudeM;
    float velocityMS;
    float accelerationMSS;

    float unfilteredNoOffsetAltitudeM;
};

struct State6D_s {
    Vector3D_s position;
    Vector3D_s velocity;
    Vector3D_s acceleration;
    Vector3D_s orientation;
    Vector3D_s angularVelocity;
};

struct RocketState_s {
    Timestamp_s timestamp;
    Coordinates_s rawGps;
    FlightState_e flightState;
    State1D_s state1D;
    State6D_s state6D;
};

enum AxisDirection : int32_t {
    ERROR_AXIS_DIRECTION,
    POS_X,
    NEG_X,
    POS_Y,
    NEG_Y,
    POS_Z,
    NEG_Z,
};

struct Quaternion {
    float w, x, y, z;
};

// This can be added in a struct definition to ensure the compiler doesn't add any padding bytes between variables
#define remove_struct_padding __attribute__((packed))

#ifndef AVIONICS_ARGUMENT_isSim
#define AVIONICS_ARGUMENT_isSim false
#endif

#ifndef AVIONICS_ARGUMENT_isDev
#define AVIONICS_ARGUMENT_isDev false
#endif

#define US_TIMER_START(id) uint32_t startTime##id = micros();
#define US_TIMER_END(id) uint32_t endTime##id = micros(); Serial.println(endTime##id - startTime##id);

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H
