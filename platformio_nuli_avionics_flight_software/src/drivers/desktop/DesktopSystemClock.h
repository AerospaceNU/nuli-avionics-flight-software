#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPSYSTEMCLOCK_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPSYSTEMCLOCK_H

#include <chrono>
#include "Avionics.h"
#include "core/generic_hardware/SystemClock.h"

class DesktopSystemClock final : public SystemClock {
public:
    DesktopSystemClock() {
        startTime = std::chrono::steady_clock::now();
    }

    uint32_t currentRuntimeMs() override {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        return static_cast<uint32_t>(elapsed);
    }

    uint32_t currentRuntimeUs() override {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime).count();
        return static_cast<uint32_t>(elapsed);
    }

private:
    std::chrono::steady_clock::time_point startTime;
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPSYSTEMCLOCK_H
