#ifndef DESKTOP_HARDWAREMANAGER_H
#define DESKTOP_HARDWAREMANAGER_H

#include "Avionics.h"
#include "generic_hardware/GenericHardware.h"

/**
 * This macro generates three methods for a given type of hardware:
 * - addHardware(Hardware*);
 * - getNumHardware();
 * - getHardwareArray();
 *
 * ## works as string concatenation in macros, allowing for the type name to be injected into the method names
 */
#define GENERATE_GET_ADD_METHODS_MACRO(Type, arrayVariable, numVariable, MaxNum)    \
int16_t append##Type(Type* instance) {                                              \
    if(numVariable >= MaxNum) return -1;                                            \
    arrayVariable[numVariable] = instance;                                          \
    numVariable++;                                                                  \
    return numVariable - 1;                                                         \
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
    void setLoopRateHz(uint32_t loopRate);

    /**
     * @brief Sets up all hardware
     * @details Calls setup for each sensor, communication link, etc
     */
    void setup(DebugStream *debugStream, SystemClock *systemClock, uint32_t loopRateHz);

    /**
     * @brief Reads in all sensor data
     * @details Calls read for each sensor, the resultant data is stored within each sensor object
     */
    void readSensors() const;

    Timestamp_s enforceLoopTime();

    uint32_t getTargetLoopTimeMs() const;

    DebugStream* getDebugStream() const;

    GENERATE_GET_ADD_METHODS_MACRO(FramMemory, m_framMemoryArray, m_numFramMemory, MAX_FRAM_MEMORY_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(Pyro, m_pyroArray, m_numPyros, MAX_PYRO_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(VoltageSensor, m_voltageSensorArray, m_numVoltageSensors, MAX_VOLTAGE_SENSOR_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(Barometer, m_barometerArray, m_numBarometers, MAX_BAROMETER_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(Accelerometer, m_accelerometerArray, m_numAccelerometers, MAX_ACCELEROMETER_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(Magnetometer, m_magnetometerArray, m_numMagnetometers, MAX_MAGNETOMETER_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(Gyroscope, m_gyroscopeArray, m_numGyroscopes, MAX_GYROSCOPE_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(FlashMemory, m_flashMemoryArray, m_numFlashMemory, MAX_FLASH_MEMORY_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(RadioLink, m_radioLinkArray, m_numRadioLinks, MAX_RADIO_TRANSMITTER_LINK_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(Indicator, m_indicatorArray, m_numIndicators, MAX_INDICATOR_NUM)

    GENERATE_GET_ADD_METHODS_MACRO(GenericSensor, m_genericSensorArray, m_numGenericSensors, MAX_GENERIC_SENSOR_NUM)

private:
    uint32_t m_loopTime = 10;
    uint32_t m_loopDtMs = 0; ///< Tracks the loop execution time
    uint32_t m_currentLoopTimestampMs = 0; ///< Tracks the start time of each loop
    uint32_t m_tickCount = 0;
    uint32_t m_intendedTickEndtimeUs = 0;

    SystemClock* m_systemClock = nullptr; ///< System clocks
    DebugStream* m_debug = nullptr; ///< Debug stream

    uint8_t m_numPyros = 0; ///< Number of Pyros in the system
    uint8_t m_numVoltageSensors = 0; ///< Number of VoltageSensors in the system
    uint8_t m_numBarometers = 0; ///< Number of Barometers in the system
    uint8_t m_numAccelerometers = 0; ///< Number of Accelerometers in the system
    uint8_t m_numMagnetometers = 0; ///< Number of Magnetometers in the system
    uint8_t m_numGyroscopes = 0; ///< Number of Gyroscopes in the system
    uint8_t m_numFlashMemory = 0; ///< Number of FlashMemory in the system
    uint8_t m_numFramMemory = 0; ///< Number of FlashMemory in the system
    uint8_t m_numRadioLinks = 0; ///< Number of RadioLinks in the system
    uint8_t m_numIndicators = 0; ///< Number of RadioLinks in the system
    uint8_t m_numGenericSensors = 0; ///< Number of generic sensors
    Pyro* m_pyroArray[MAX_PYRO_NUM] = {nullptr}; ///< Array containing all the Pyros in the system
    VoltageSensor* m_voltageSensorArray[MAX_VOLTAGE_SENSOR_NUM] = {nullptr}; ///< Array containing all the VoltageSensors in the system
    Barometer* m_barometerArray[MAX_BAROMETER_NUM] = {nullptr}; ///< Array containing all the Barometers in the system
    Accelerometer* m_accelerometerArray[MAX_ACCELEROMETER_NUM] = {nullptr}; ///< Array containing all the Accelerometers in the system
    Magnetometer* m_magnetometerArray[MAX_BAROMETER_NUM] = {nullptr}; ///< Array containing all the Magnetometers in the system
    Gyroscope* m_gyroscopeArray[MAX_GYROSCOPE_NUM] = {nullptr}; ///< Array containing all the Gyroscopes in the system
    FlashMemory* m_flashMemoryArray[MAX_FLASH_MEMORY_NUM] = {nullptr}; ///< Array containing all the FlashMemory in the system
    FramMemory* m_framMemoryArray[MAX_FRAM_MEMORY_NUM] = {nullptr}; ///< Array containing all the FramMemory in the system
    RadioLink* m_radioLinkArray[MAX_RADIO_TRANSMITTER_LINK_NUM] = {nullptr}; ///< Array containing all the RadioLinks in the system
    Indicator* m_indicatorArray[MAX_INDICATOR_NUM] = {nullptr}; ///< Array containing all the RadioLinks in the system
    GenericSensor* m_genericSensorArray[MAX_GENERIC_SENSOR_NUM] = {nullptr}; ///< Array containing all generic sensors
};

#undef GENERATE_GET_ADD_METHODS_MACRO   // Macro has no use beyond this file
#endif //DESKTOP_HARDWAREMANAGER_H
