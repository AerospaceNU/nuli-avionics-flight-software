#include "ArduinoWatchdog.h"
#include <Arduino.h>
#include <Adafruit_SleepyDog.h>

namespace {
    // RCAUSE.WDT is bit 5 on both SAMD21 (PM) and SAMD51 (RSTC)
    constexpr uint8_t RCAUSE_WDT_BIT = 0x20;
}

void ArduinoWatchdog::setup(DebugStream* debugStream) {
    if (causedLastReset()) {
        debugStream->error("MCU reset was caused by the watchdog timing out");
    }
}

void ArduinoWatchdog::enable(const uint32_t timeoutMs) {
    m_timeoutMs = (uint32_t)Watchdog.enable((int)timeoutMs);
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
