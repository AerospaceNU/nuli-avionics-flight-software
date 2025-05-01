#ifndef DESKTOP_HARDWAREMANAGER_H
#define DESKTOP_HARDWAREMANAGER_H

#include "Avionics.h"
#include "generic_hardware/GenericSensor.h"
#include "generic_hardware/Barometer.h"
#include "generic_hardware/Accelerometer.h"
#include "generic_hardware/GPS.h"
#include "generic_hardware/Gyroscope.h"
#include "generic_hardware/Magnetometer.h"
#include "generic_hardware/Pyro.h"
#include "generic_hardware/FlashMemory.h"
#include "generic_hardware/RadioLink.h"
#include "generic_hardware/SystemClock.h"
#include "generic_hardware/DebugStream.h"
#include "generic_hardware/VoltageSensor.h"


/**
 * This macro generates three methods for a given type of hardware:
 * - addHardware(Hardware*);
 * - getNumHardware();
 * - getHardwareArray();
 *
 * ## works as string concatenation in macros, allowing for the type name to be injected into the method names
 */
#define GENERATE_GET_ADD_METHODS_MACRO(Type, arrayVariable, numVariable, MaxNum)    \
void add##Type(Type* instance) {                                                    \
    if(numVariable >= MaxNum) return;                                               \
    arrayVariable[numVariable] = instance;                                          \
    numVariable++;                                                                  \
}                                                                                   \
inline uint8_t getNum##Type##s() const {                                            \
    return numVariable;                                                             \
}                                                                                   \
inline Type* get##Type(uint8_t index) {                                             \
    return arrayVariable[index];                                                    \
}

#define GENERATE_GET_SET_METHODS_MACRO(Type, arrayVariable)                         \
void set##Type(Type* instance) {                                                    \
    arrayVariable = instance;                                                       \
}                                                                                   \
inline Type* get##Type() {                                                          \
    return arrayVariable;                                                           \
}


/**
 * @class HardwareAbstraction
 * @brief Provide an abstraction to access all hardware
 */
class HardwareAbstraction {
public:
    /**
     * @brief Sets up all hardware
     * @details Calls setup for each sensor, communication link, etc
     */
    void setup();

    /**
     * @brief Reads in all sensor data
     * @details Calls read for each sensor, the resultant data is stored within each sensor object
     */
    void readAllSensors();

    /**
     * @brief Reads all communications links to buffers
     * @details Calls read on each communication link, and does something???? with the data
     */
    void readAllRadioLinks();

    inline void delay(uint32_t time) {
        uint32_t end = getRuntimeMs() + time;
        while (getRuntimeMs() < end);
    }

    /**
     * @brief Called at the beginning of each loop to track change in time between loops
     * @details This should only be called in once, and only in the Core
     */
    inline void updateLoopTimestamp() {
        uint_avionics_time_t lastTime = m_currentLoopTimestampMs;
        m_currentLoopTimestampMs = getRuntimeMs();
        m_loopDtMs = m_currentLoopTimestampMs - lastTime;
    }

    /**
     * Get the runtime at the start of this loop
     * @return time in ms
     */
    inline uint_avionics_time_t getLoopTimestampMs() const {
        return m_currentLoopTimestampMs;
    }

    /**
     * @brief Gets the time since the start of the last loop
     * @return Loop time in ms
     */
    inline uint_avionics_time_t getLoopDtMs() const {
        return m_loopDtMs;
    }

    /**
     * @brief Gets the runtime of the system
     * @return runtime in ms
     */
    inline uint32_t getRuntimeMs() {
        if (m_systemClock != nullptr) {
            return m_systemClock->currentRuntimeMs();
        }
        return 0;
    }

    GENERATE_GET_ADD_METHODS_MACRO(Pyro, m_pyroArray, m_numPyros, MAX_PYRO_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(VoltageSensor, m_voltageSensorArray, m_numVoltageSensors, MAX_VOLTAGE_SENSOR_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(Barometer, m_barometerArray, m_numBarometers, MAX_BAROMETER_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(Accelerometer, m_accelerometerArray, m_numAccelerometers, MAX_ACCELEROMETER_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(Magnetometer, m_magnetometerArray, m_numMagnetometers, MAX_MAGNETOMETER_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(Gyroscope, m_gyroscopeArray, m_numGyroscopes, MAX_GYROSCOPE_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(GPS, m_gpsArray, m_numGps, MAX_GPS_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(FlashMemory, m_flashMemoryArray, m_numFlashMemory, MAX_FLASH_MEMORY_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(RadioLink, m_radioLinkArray, m_numRadioLinks, MAX_RADIO_TRANSMITTER_LINK_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(GenericSensor, m_genericSensorArray, m_numGenericSensors, MAX_GENERIC_SENSOR_NUM)

//    GENERATE_GET_ADD_METHODS_MACRO(Configuration, m_configurationArray, m_numConfigurations, MAX_CONFIGURATION_NUM)

    GENERATE_GET_SET_METHODS_MACRO(SystemClock, m_systemClock)

    GENERATE_GET_SET_METHODS_MACRO(DebugStream, m_debugStream)

private:
    uint32_t m_currentLoopTimestampMs = 0;              ///< Tracks the start time of each loop
    uint32_t m_loopDtMs = 0;                            ///< Tracks the loop execution time

    uint8_t m_numPyros = 0;                             ///< Number of Pyros in the system
    uint8_t m_numVoltageSensors = 0;                    ///< Number of VoltageSensors in the system
    uint8_t m_numBarometers = 0;                        ///< Number of Barometers in the system
    uint8_t m_numAccelerometers = 0;                    ///< Number of Accelerometers in the system
    uint8_t m_numMagnetometers = 0;                     ///< Number of Magnetometers in the system
    uint8_t m_numGyroscopes = 0;                        ///< Number of Gyroscopes in the system
    uint8_t m_numGps = 0;                               ///< Number of Gps in the system
    uint8_t m_numFlashMemory = 0;                       ///< Number of FlashMemory in the system
    uint8_t m_numRadioLinks = 0;                        ///< Number of RadioLinks in the system
    uint8_t m_numGenericSensors = 0;                    ///< Number of generic sensors
    uint8_t m_numConfigurations = 0;                    ///< Number of generic sensors
    Pyro* m_pyroArray[MAX_PYRO_NUM] = {nullptr};                                ///< Array containing all the Pyros in the system
    VoltageSensor* m_voltageSensorArray[MAX_VOLTAGE_SENSOR_NUM] = {nullptr};    ///< Array containing all the VoltageSensors in the system
    Barometer* m_barometerArray[MAX_BAROMETER_NUM] = {nullptr};                 ///< Array containing all the Barometers in the system
    Accelerometer* m_accelerometerArray[MAX_ACCELEROMETER_NUM] = {nullptr};     ///< Array containing all the Accelerometers in the system
    Magnetometer* m_magnetometerArray[MAX_BAROMETER_NUM] = {nullptr};           ///< Array containing all the Magnetometers in the system
    Gyroscope* m_gyroscopeArray[MAX_GYROSCOPE_NUM] = {nullptr};                 ///< Array containing all the Gyroscopes in the system
    GPS* m_gpsArray[MAX_GPS_NUM] = {nullptr};                                   ///< Array containing all the Gps in the system
    FlashMemory* m_flashMemoryArray[MAX_FLASH_MEMORY_NUM] = {nullptr};          ///< Array containing all the FlashMemory in the system
    RadioLink* m_radioLinkArray[MAX_RADIO_TRANSMITTER_LINK_NUM] = {nullptr};        ///< Array containing all the RadioLinks in the system
    GenericSensor* m_genericSensorArray[MAX_GENERIC_SENSOR_NUM] = {nullptr};                    ///< Array containing all generic sensors
    GenericSensor* m_configurationArray[MAX_CONFIGURATION_NUM] = {nullptr};                    ///< Array containing all generic sensors
    SystemClock* m_systemClock = nullptr;                                                       ///< System clocks
    DebugStream* m_debugStream = nullptr;                                                       ///< Debug stream
    DebugStream voidDump;
};

#undef GENERATE_GET_ADD_METHODS_MACRO   // Macro has no use beyond this file
#undef GENERATE_GET_SET_METHODS_MACRO
#endif //DESKTOP_HARDWAREMANAGER_H
