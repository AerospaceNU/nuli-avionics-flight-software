#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONMEMORY_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONMEMORY_H

#include "core/generic_hardware/DebugStream.h"
#include "GenericAvionicsHardware.h"
#include <cstring>
#include <cstdint>

class FramMemory: public GenericAvionicsHardware {
public:
    virtual ~FramMemory() = default;
    virtual void setup(DebugStream* debugStream, WatchdogTimer* watchdog) {};

    virtual void write(uint32_t address, const uint8_t* buffer, uint32_t length) = 0;

    virtual void read(uint32_t address, uint8_t* buffer, uint32_t length) = 0;
};

/**
 * @class VolatileConfigurationMemory
 * @brief For board without non-volatile memory for configuration
 * @details Lets the configuration API stay guaranteed-available on boards with no real non-volatile memory - resets to defaults every restart though.
 * @tparam N Size of the memory
 */
template <unsigned N>
class VolatileConfigurationMemory : public FramMemory {
public:
    void write(uint32_t address, const uint8_t* buffer, uint32_t length) override {
        if (length > N) return;
        memcpy(m_buffer, buffer, length);
    }

    void read(uint32_t address, uint8_t* buffer, uint32_t length) override {
        if (length > N) return;
        memcpy(buffer, m_buffer, length);
    }

private:
    uint8_t m_buffer[N]{};
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONMEMORY_H
