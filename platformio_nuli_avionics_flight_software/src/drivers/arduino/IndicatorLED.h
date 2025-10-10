#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATORLED_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATORLED_H

#include "Avionics.h"
#include <Arduino.h>
#include "../../core/generic_hardware/Indicator.h"

class IndicatorLED final : public Indicator {
public:
    explicit IndicatorLED(const int8_t pin) {
        m_pin = pin;
    }

    void setOutputPercent(float power) {
        if (power <= 100 && power >= 0) {
            power *= 2.55;
            m_outputPower = (int16_t) power;
        }
    }

    void setup(DebugStream *debugStream) override {
        pinMode(m_pin, OUTPUT);
    }

    void on() override {
        if (m_outputPower > 0) {
            analogWrite(m_pin, m_outputPower);
        } else {
            digitalWrite(m_pin, HIGH);
        }
    }

    void off() override {
        if (m_outputPower > 0) {
            analogWrite(m_pin, 0);
        } else {
            digitalWrite(m_pin, LOW);
        }
    }

private:
    int8_t m_pin;
    int16_t m_outputPower = -1;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATORLED_H
