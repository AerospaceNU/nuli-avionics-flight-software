#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOWATCHDOG_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOWATCHDOG_H

#include "Avionics.h"
#include "core/generic_hardware/GenericHardware.h"

/** @brief SAMD21/SAMD51 watchdog via Adafruit_SleepyDog, which handles the chips' different watchdog clocking */
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
