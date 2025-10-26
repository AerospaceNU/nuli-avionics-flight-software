#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPSYSTEMCLOCK_HDummy
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPSYSTEMCLOCK_HDummy

#include <chrono>
#include "Avionics.h"
#include "core/generic_hardware/SystemClock.h"

class DummySystemClock final : public SystemClock {
public:
    DummySystemClock(float rate) {
        rate = 1.0f / rate;
        rate *= 1000;
        dt = rate;
    }

    uint32_t currentRuntimeMs() override {
        ms += dt;
        return ms;
    }

    uint32_t currentRuntimeUs() override {
        us += dt * Units::MS_TO_US;
        return us;
    }

private:
    uint32_t dt = 0;
    uint32_t ms = 0;
    uint32_t us = 0;
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPSYSTEMCLOCK_HDummy
