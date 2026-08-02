#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H

#include "Avionics.h"
#include "Arduino.h"
#include "core/generic_hardware/GenericHardware.h"
#include "core/generic_hardware/WatchdogTimer.h"

// N is the line buffer capacity (including the null terminator) for readLine()/getLine().
template <unsigned N>
class SerialDebug final : public DebugStream {
public:
    // watchdog is optional (null where there's none, e.g. ground station) - pet while waiting so an
    // unattended dev build (no terminal open yet) doesn't get reset-looped by a stale armed watchdog.
    // echo, if set, immediately sends each received character back out over Serial as it's read.
    explicit SerialDebug(const bool waitToConnect = false, WatchdogTimer* watchdog = nullptr, const bool echo = false) :
        m_wait(waitToConnect), m_watchdog(watchdog), m_echo(echo) {}

    void setup() override {
        Serial.begin(115200);
        while (m_wait && !Serial) {
            if (m_watchdog) m_watchdog->petInLoop();
        }
    }

    size_t write(const void* buffer, const size_t size) override {
        return Serial.write((char*)buffer, size);
    }

    bool readLine() override {
        while (Serial.available() > 0) {
            const char c = Serial.read();
            if (m_serialReadIndex < N - 1) {
                m_serialRead[m_serialReadIndex++] = c;
                if (m_echo) {
                    Serial.print(c);
                }
            }
            if (c == '\n') {
                m_serialRead[m_serialReadIndex] = '\0';
                m_serialReadIndex = 0;
                return true;
            }
        }
        return false;
    }

    char* getLine() override {
        return m_serialRead;
    }

private:
    bool m_wait;
    WatchdogTimer* m_watchdog;
    bool m_echo;
    char m_serialRead[N] = "";
    uint32_t m_serialReadIndex = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
