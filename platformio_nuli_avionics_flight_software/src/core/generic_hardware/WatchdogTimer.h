#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_WATCHDOGTIMER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_WATCHDOGTIMER_H

#include "Avionics.h"
#include "core/generic_hardware/DebugStream.h"

/** @brief Resets the MCU if pet() isn't called often enough after enable(). Base methods are no-ops. */
class WatchdogTimer {
public:
    virtual ~WatchdogTimer() = default;

    virtual void setup(DebugStream* debugStream) {}

    /** @param timeoutMs Max time between pet() calls before reset. */
    virtual void enable(uint32_t timeoutMs) {}

    virtual void pet() {}

    virtual void disable() {}

    /** @brief Valid immediately after boot. */
    virtual bool causedLastReset() const { return false; }

    // For hot loops: pets at most once per petInLoopIntervalMs(), since pet() can be costly
    void petInLoop() {
        const uint32_t nowMs = currentTimeMs();
        if (nowMs - m_lastPetInLoopMs >= petInLoopIntervalMs()) {
            pet();
            m_lastPetInLoopMs = nowMs;
        }
    }

protected:
    virtual uint32_t currentTimeMs() const { return 0; }

    // Default 0 pets every call
    virtual uint32_t petInLoopIntervalMs() const { return 0; }

private:
    uint32_t m_lastPetInLoopMs = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_WATCHDOGTIMER_H
