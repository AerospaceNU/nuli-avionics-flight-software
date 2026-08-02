#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H

#include "Avionics.h"
#include "Arduino.h"
#include "core/generic_hardware/GenericHardware.h"
#include "core/generic_hardware/WatchdogTimer.h"

class SerialDebug final : public DebugStream {
public:
    // watchdog is optional (null where there's none, e.g. ground station) - pet while waiting so an
    // unattended dev build (no terminal open yet) doesn't get reset-looped by a stale armed watchdog.
    explicit SerialDebug(const bool waitToConnect = false, WatchdogTimer* watchdog = nullptr) :
        m_wait(waitToConnect), m_watchdog(watchdog) {}

    void setup() override {
        Serial.begin(115200);
        while (m_wait && !Serial) {
            if (m_watchdog) m_watchdog->petInLoop();
        }
    }

    size_t write(const void* buffer, const size_t size) override {
        return Serial.write((char*)buffer, size);
    }

private:
    bool m_wait;
    WatchdogTimer* m_watchdog;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
