

#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H

#include "Avionics.h"
#include "ConstantsUnits.h"
#include "core/generic_hardware/DebugStream.h"

class SerialDebug : public DebugStream {
public:
    explicit SerialDebug(bool wait = true) {
        m_wait = wait;
    }

    void setup() override {
        Serial.begin(9600);
        while (m_wait && !Serial);
    }

    void print(const char* str) override {
        Serial.print(str);
    }

    void print(int64_t num) override {
        Serial.print(num);
    }

    void print(double num) override {
        Serial.print(num);
    }
private:
    bool m_wait;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
