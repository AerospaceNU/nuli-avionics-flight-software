#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H

#include <cstdint>
#include <cmath>

/**
 * @file Avionics.h
 * @brief Global definitions for the codebase
 */

// Define state transition paramiters
/**
 * Acceleration threshold design:
 *      - Rockets must have a thrust to weight ratio of at least 3 (4+ is ideal). F = thrust force, r = ratio:
 *          - F = rmg, F - mg = ma, rmg - mg = ma, a = (r - 1)g
 *      - Worst case (lowest) acceleration: a = (3-1)g = ~20 m/ss
 *      - This is not very high, and can be easily achieved by shaking the device
 */
// Launch detection
constexpr float LAUNCH_ACCELERATION_THRESHOLD_MSS = 20.0; ///, FCB used 20 m/s^2. This
constexpr float LAUNCH_ALTITUDE_THRESHOLD_M = 100.0;
constexpr uint32_t LAUNCH_DEBOUNCE_TIMER_MS = 150;
// Apogee detection
constexpr uint32_t APOGEE_DEBOUNCE_TIMER_MS = 150;
constexpr float APOGEE_ALTITUDE_CHANGE_THRESHOLD_M = 2.0;
// Landing detection
constexpr float LANDING_ALTITUDE_CHANGE_THRESHOLD_M = 3.0;
constexpr uint32_t LANDING_DEBOUNCE_TIMER_MS = 3000;
// On boot state detection
constexpr uint32_t UNKNOWN_STATE_TIMER_MS = 1000;
constexpr float UNKNOWN_STATE_ALTITUDE_CHANGE_THRESHOLD_M = 5.0;
constexpr float UNKNOWN_STATE_VELOCITY_THRESHOLD_MS = 3.0;


// Hardware Abstraction max size parameters
constexpr uint8_t MAX_PYRO_NUM = 10;
constexpr uint8_t MAX_VOLTAGE_SENSOR_NUM = 5;
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
constexpr uint16_t MAX_CONFIGURATION_LENGTH = 500;


/**
 * @struct Vector3D_s
 * @brief Vector with 3 axis
 */
struct Vector3D_s {
    float x; ///< X axis
    float y; ///< Y axis
    float z; ///< Z axis
};

struct Quaternion_s {
    float x;
    float y;
    float z;
    float w;
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

struct Orientation_s {
    Vector3D_s angle;
    Vector3D_s angularVelocity;
    Quaternion_s angleQuaternion;
};

struct State6D_s {
    Vector3D_s position;
    Vector3D_s velocity;
    Vector3D_s acceleration;
};

struct RocketState_s {
    Timestamp_s timestamp;
    Coordinates_s rawGps;
    FlightState_e flightState;
    State1D_s state1D;
    Orientation_s orientation;
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

struct GyroscopeBias_s {
    Vector3D_s bias[MAX_GYROSCOPE_NUM] = {};

    GyroscopeBias_s() {
        for (auto& v : bias) v = {0, 0, 0};
    }
};

// This can be added in a struct definition to ensure the compiler doesn't add any padding bytes between variables
#define remove_struct_padding __attribute__((packed))

#ifndef AVIONICS_ARGUMENT_isSim
#define AVIONICS_ARGUMENT_isSim false
#endif

#ifndef AVIONICS_ARGUMENT_isDev
#define AVIONICS_ARGUMENT_isDev false
#endif


#ifdef PLATFORMIO
#define US_TIMER_START(id) uint32_t startTime##id = micros();
#define US_TIMER_END(id) uint32_t endTime##id = micros(); Serial.println(endTime##id - startTime##id);
#else
void setup();
void loop();
#define AVIONICS_DESKTOP_MAIN  int main() { setup(); while (true) loop(); }
#endif

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H
