#ifndef DESKTOP_HARDWAREMANAGER_H
#define DESKTOP_HARDWAREMANAGER_H

#include "Avionics.h"
#include "generic_hardware/GenericHardware.h"
#include <etl/vector.h>

#define GENERATE_METHODS_MACRO(Type, MaxNum)                                                            \
protected:                                                                                              \
etl::vector<Type*, MaxNum> Type##Storage;                                                               \
public:                                                                                                 \
int16_t append##Type(Type* instance) {                                                                  \
    if(Type##Storage.full() || m_allHardware.full()) {                                                  \
        if(Type##Storage.full()) avionicsSystemError(""#Type" storage limit reached" );                 \
        if(m_allHardware.full()) avionicsSystemError("HardwareAbstraction storage limit reached");      \
        return -1;                                                                                      \
    }                                                                                                   \
    Type##Storage.push_back(instance);                                                                  \
    m_allHardware.push_back(instance);                                                                  \
    return Type##Storage.size() - 1;                                                                    \
}                                                                                                       \
inline uint8_t getNum##Type##s() const {                                                                \
    return Type##Storage.size();                                                                        \
}                                                                                                       \
inline Type* get##Type(uint8_t index) {                                                                 \
    return Type##Storage[index];                                                                        \
}

/**
 * @class HardwareAbstraction
 * @brief Provide an abstraction to access all hardware
 */
class HardwareAbstraction {
public:
    HardwareAbstraction(DebugStream& debugStream, SystemClock& systemClock, uint32_t loopRateHz);

    void setup();

    void setLoopRateHz(uint32_t loopRate);

    void runAndReadAllHardware() const;

    Timestamp_s enforceLoopTime();

    uint32_t getTargetLoopTimeMs() const;

    DebugStream* getDebugStream() const;

    void avionicsSystemError(const char *message) const;

    void appendGenericHardware(GenericAvionicsHardware* instance) {
        if (m_allHardware.full()) return;
        m_allHardware.push_back(instance);
    }

    GENERATE_METHODS_MACRO(FramMemory, MAX_FRAM_MEMORY_NUM)
    GENERATE_METHODS_MACRO(Pyro, MAX_PYRO_NUM)
    GENERATE_METHODS_MACRO(VoltageSensor, MAX_VOLTAGE_SENSOR_NUM)
    GENERATE_METHODS_MACRO(Barometer, MAX_BAROMETER_NUM)
    GENERATE_METHODS_MACRO(Accelerometer, MAX_ACCELEROMETER_NUM)
    GENERATE_METHODS_MACRO(Magnetometer, MAX_MAGNETOMETER_NUM)
    GENERATE_METHODS_MACRO(Gyroscope, MAX_GYROSCOPE_NUM)
    GENERATE_METHODS_MACRO(FlashMemory, MAX_FLASH_MEMORY_NUM)
    GENERATE_METHODS_MACRO(RadioLink, MAX_RADIO_TRANSMITTER_LINK_NUM)
    GENERATE_METHODS_MACRO(Indicator, MAX_INDICATOR_NUM)
    GENERATE_METHODS_MACRO(DigitalInput, MAX_DIGITAL_INPUT_NUM)

private:
    uint32_t m_loopTime = 10;
    uint32_t m_loopDtMs = 0; ///< Tracks the loop execution time
    uint32_t m_currentLoopTimestampMs = 0; ///< Tracks the start time of each loop
    uint32_t m_tickCount = 0;
    uint32_t m_intendedTickEndtimeUs = 0;

    SystemClock* m_systemClock = nullptr; ///< System clocks
    DebugStream* m_debug = nullptr; ///< Debug stream

    etl::vector<GenericAvionicsHardware*, MAX_GENERIC_HARDWARE_NUM> m_allHardware;
};

#undef GENERATE_METHODS_MACRO   // Macro has no use beyond this file
#endif //DESKTOP_HARDWAREMANAGER_H
