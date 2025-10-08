#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H

#include "Avionics.h"
#include "ConstantsUnits.h"
#include "core/generic_hardware/DebugStream.h"

class SerialDebug : public DebugStream {
public:
    explicit SerialDebug(bool waitToConnect = true) : m_wait(waitToConnect) {}

    void setup() override {
        Serial.begin(9600);
        while (m_wait && !Serial);
    }

    // ---- Print ----
    void print(const char* str) override { Serial.print(str); }
    void print(char c) override { Serial.print(c); }
    void print(int32_t num) override { Serial.print(num); }
    void print(uint32_t num) override { Serial.print(num); }
    void print(double num) override { Serial.print(num, m_decimalPlaces); }

    // ---- Println ----
    void println() override { Serial.println(); }
    void println(const char* str) override { Serial.println(str); }
    void println(char c) override { Serial.println(c); }
    void println(int32_t num) override { Serial.println(num); }
    void println(uint32_t num) override { Serial.println(num); }
    void println(double num) override {
        print(num);
        Serial.println();
    }

    // ---- Write ----
    size_t write(uint8_t b) override { return Serial.write(b); }
    size_t write(const void* buffer, size_t size) override { return Serial.write((char *)buffer, size); }

private:
    bool m_wait;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
