#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPSYSTEMCLOCK_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPSYSTEMCLOCK_H

#include "Avionics.h"
#include "ConstantsUnits.h"
#include "core/generic_hardware/GenericHardware.h"

/**
 * @class DesktopSystemClock
 * @brief Simulated time, set by the caller before each loop so the loop never waits
 */
class DesktopSystemClock final : public SystemClock {
public:
    void setRuntimeMs(const uint32_t runtimeMs) {
        m_runtimeMs = runtimeMs;
    }

    uint32_t currentRuntimeMs() override {
        return m_runtimeMs;
    }

    uint32_t currentRuntimeUs() override {
        return m_runtimeMs * Units::MS_TO_US;
    }

private:
    uint32_t m_runtimeMs = 0;
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPSYSTEMCLOCK_H
