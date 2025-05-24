#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATORLED_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATORLED_H

#include "Avionics.h"
#include <Arduino.h>
#include "../../core/generic_hardware/Indicator.h"

class IndicatorLED : public Indicator {
public:
    explicit IndicatorLED(int8_t pin) {
        m_pin = pin;
    }

    void setup() override {
        pinMode(m_pin, OUTPUT);
    }

    void on() override {
        digitalWrite(m_pin, HIGH);
    }

    void off() override {
        digitalWrite(m_pin, LOW);
    }

private:
    int8_t m_pin;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATORLED_H
