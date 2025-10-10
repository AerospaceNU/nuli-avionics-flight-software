#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H

#include "Avionics.h"
#include "Arduino.h"
#include "core/generic_hardware/DebugStream.h"

class SerialDebug final : public DebugStream {
public:
    explicit SerialDebug(const bool waitToConnect = false) : m_wait(waitToConnect) {}

    void setup() override {
        Serial.begin(115200);
        while (m_wait && !Serial);
    }

    size_t write(const void* buffer, const size_t size) override {
        return Serial.write((char*)buffer, size);
    }

private:
    bool m_wait;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
