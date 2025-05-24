#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATORBUZZER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATORBUZZER_H


#include "Avionics.h"
#include <Arduino.h>
#include "../../core/generic_hardware/Indicator.h"

class IndicatorBuzzer : public Indicator {
public:
    explicit IndicatorBuzzer(int8_t pin, uint32_t frequency) {
        m_pin = pin;
        m_frequency = frequency;
    }

    void on() override {
        if(!m_active) {
            m_active = true;
            tone(m_pin, m_frequency);
        }
    }

    void off() override {
        noTone(m_pin);
        m_active = false;
    }

private:
    int8_t m_pin;
    uint32_t m_frequency;
    bool m_active = false;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATORBUZZER_H
