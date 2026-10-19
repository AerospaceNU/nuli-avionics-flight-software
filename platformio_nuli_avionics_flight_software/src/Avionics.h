#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H

#include <cstdint>
#include <cmath>
#include <Quaternion.h>

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
// Burnout
constexpr uint32_t BURNOUT_DEBOUNCE_TIMER_MS = 150;
// Apogee detection
constexpr uint32_t APOGEE_DEBOUNCE_TIMER_MS = 150;
constexpr float APOGEE_ALTITUDE_CHANGE_THRESHOLD_M = 2.0;
constexpr uint32_t MACH_LOCKOUT_TIMER_MS = 5000;
constexpr float MACH_LOCKOUT_THRESHOLD_MS = 150.0;
// Barometer de-weighting near Mach (tuned on 11 real transonic flights, 2026-10-18)
constexpr float MACH_RAMP_START_MS = 200.0;
constexpr float MACH_RAMP_END_MS = 300.0;
constexpr float MACH_BAROMETER_COVARIANCE_SCALE = 10.0;
// Landing detection
constexpr float LANDING_ALTITUDE_CHANGE_THRESHOLD_M = 3.0;
constexpr uint32_t LANDING_DEBOUNCE_TIMER_MS = 3000;
// Main deployment
constexpr uint32_t MAIN_DEPLOY_DEBOUNCE_TIMER_MS = 200;
// On boot state detection
constexpr uint32_t UNKNOWN_STATE_TIMER_MS = 1000;
constexpr float UNKNOWN_STATE_ALTITUDE_CHANGE_THRESHOLD_M = 5.0;
constexpr float UNKNOWN_STATE_VELOCITY_THRESHOLD_MS = 3.0;
constexpr uint32_t UNKNOWN_STATE_HIGH_TIMER_MS = 10000;
constexpr float UNKNOWN_STATE_SUBSONIC_ACCELERATION_MSS = 3.0;
constexpr float UNKNOWN_STATE_SUBSONIC_VELOCITY_MS = 75.0;
constexpr float UNKNOWN_STATE_SUBSONIC_RESIDUAL_M = 5.0;
constexpr float UNKNOWN_STATE_SUBSONIC_MAX_ALTITUDE_M = 9000.0;
constexpr float UNKNOWN_STATE_SUBSONIC_MIN_VALID_FRACTION = 0.8;
constexpr uint32_t GROUND_REFERENCE_UPDATE_DELAY = 2000;
constexpr float GROUND_REFERENCE_FREEZE_SPEED_MS = 15.0; ///< no ground re-take faster than this: a slow boost would drag the ground up
constexpr uint32_t BAROMETER_OUTAGE_MS = 1000; ///< outage after which the Kalman filter restarts from the next valid reading

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
constexpr uint8_t MAX_GENERIC_HARDWARE_NUM = 50;
constexpr uint8_t MAX_DIGITAL_INPUT_NUM = 5;
constexpr uint8_t MAX_INDICATOR_NUM_BEEP_NUM = 4;
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

inline float vector3DMagnitude(const Vector3D_s& vector) {
    return sqrtf(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

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
    bool barometerValid; ///< False while altitude comes from a held pressure rather than a measurement
    float accelerometerMagnitudeMSS;
};

struct Orientation_s {
    float tiltMagnitudeDeg;
    Vector3D_s angularVelocity;
    Quaternion angleQuaternion;
    float roll;
    float pitch;
    float yaw;
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

// Leading byte of every RadioLink packet (both directions) - lets one physical link carry
// telemetry downlink, remote CLI commands uplink, and remote CLI responses downlink.
enum RadioMessageType_e : uint8_t {
    RADIO_MSG_TELEMETRY = 0,
    RADIO_MSG_CLI_COMMAND = 1,
    RADIO_MSG_CLI_RESPONSE = 2,
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

inline const char* axisDirectionName(const AxisDirection direction) {
    switch (direction) {
    case POS_X: return "POS_X";
    case NEG_X: return "NEG_X";
    case POS_Y: return "POS_Y";
    case NEG_Y: return "NEG_Y";
    case POS_Z: return "POS_Z";
    case NEG_Z: return "NEG_Z";
    default: return "ERROR_AXIS_DIRECTION";
    }
}

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

#ifndef AVIONICS_ARGUMENT_boardVersion
#define AVIONICS_ARGUMENT_boardVersion 1
#endif

#ifndef AVIONICS_ARGUMENT_firmwareVersion
#define AVIONICS_ARGUMENT_firmwareVersion "unversioned"
#endif

#define IS_BOARD_VERSION(version) AVIONICS_ARGUMENT_boardVersion == version

#ifdef PLATFORMIO
#include "Arduino.h"
#define US_TIMER_START(id) uint32_t startTime##id = micros();
#define US_TIMER_END(id) uint32_t endTime##id = micros(); Serial.println(endTime##id - startTime##id);
#else
void setup();
void loop();
#define AVIONICS_DESKTOP_MAIN  int main() { setup(); while (true) loop(); }
#endif
#include <string.h>
#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_AVIONICS_H
