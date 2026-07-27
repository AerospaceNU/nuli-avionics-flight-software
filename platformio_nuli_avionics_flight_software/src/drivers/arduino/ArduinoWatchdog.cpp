#include "ArduinoWatchdog.h"
#include <Arduino.h>
#include <Adafruit_SleepyDog.h> // lib_deps: adafruit/Adafruit SleepyDog Library

namespace {
    // Bit 5 flags a watchdog reset on both SAMD21 (PM->RCAUSE.bit.WDT) and SAMD51 (RSTC->RCAUSE.bit.WDT)
    // - confirmed against both chips' CMSIS headers. Adafruit_SleepyDog's resetCause() wraps whichever applies.
    constexpr uint8_t RCAUSE_WDT_BIT = 0x20;
}

void ArduinoWatchdog::setup(DebugStream* debugStream) {
    if (causedLastReset()) {
        debugStream->error("MCU reset was caused by the watchdog timing out");
    }
}

void ArduinoWatchdog::enable(const uint32_t timeoutMs) {
    Watchdog.enable((int)timeoutMs);
    m_timeoutMs = timeoutMs;
    m_enabled = true;
}

void ArduinoWatchdog::pet() {
    if (m_enabled) Watchdog.reset();
}

void ArduinoWatchdog::disable() {
    Watchdog.disable();
    m_enabled = false;
}

bool ArduinoWatchdog::causedLastReset() const {
    return (Watchdog.resetCause() & RCAUSE_WDT_BIT) != 0;
}

uint32_t ArduinoWatchdog::currentTimeMs() const {
    return millis();
}

uint32_t ArduinoWatchdog::petInLoopIntervalMs() const {
    return m_timeoutMs / 4;
}
