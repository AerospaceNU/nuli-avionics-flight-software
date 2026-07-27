#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOWATCHDOG_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOWATCHDOG_H

#include "Avionics.h"
#include "core/generic_hardware/GenericHardware.h"

/**
 * @class ArduinoWatchdog
 * @brief Watchdog driver for SAMD21/SAMD51, backed by Adafruit_SleepyDog rather than hand-rolled registers.
 * @details The two chips clock the watchdog differently; getting that wrong could hang enable() itself on a SYNCBUSY bit that never clears.
 */
class ArduinoWatchdog final : public WatchdogTimer {
public:
    void setup(DebugStream* debugStream) override;

    void enable(uint32_t timeoutMs) override;

    void pet() override;

    void disable() override;

    bool causedLastReset() const override;

protected:
    uint32_t currentTimeMs() const override;

    uint32_t petInLoopIntervalMs() const override;

private:
    bool m_enabled = false;
    uint32_t m_timeoutMs = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOWATCHDOG_H
