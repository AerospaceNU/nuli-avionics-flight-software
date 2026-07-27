#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_WATCHDOGTIMER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_WATCHDOGTIMER_H

#include "Avionics.h"
#include "core/generic_hardware/DebugStream.h"

/**
 * @class WatchdogTimer
 * @brief Resets the MCU if pet() isn't called often enough after enable().
 * @details Fixed part of HardwareAbstraction (like SystemClock/DebugStream), passed to every device's setup(). All methods default to no-ops, so it's always safe to call.
 */
class WatchdogTimer {
public:
    virtual ~WatchdogTimer() = default;

    /** @brief Per-device setup hook. Default no-op. */
    virtual void setup(DebugStream* debugStream) {}

    /** @brief Arms the watchdog. @param timeoutMs Max time between pet() calls before reset. */
    virtual void enable(uint32_t timeoutMs) {}

    /** @brief Feeds the watchdog, postponing the reset. */
    virtual void pet() {}

    /** @brief Disables the watchdog. */
    virtual void disable() {}

    /** @brief Whether the last MCU reset was caused by this watchdog. Valid immediately after boot. */
    virtual bool causedLastReset() const { return false; }
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_WATCHDOGTIMER_H
