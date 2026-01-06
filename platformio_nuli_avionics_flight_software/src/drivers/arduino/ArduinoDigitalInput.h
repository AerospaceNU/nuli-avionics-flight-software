#ifndef ADIGITALPIN_H
#define ADIGITALPIN_H


#include "Avionics.h"
#include "Arduino.h"
#include "core/generic_hardware/GenericHardware.h"

class ArduinoDigitalInput final : public DigitalInput {
public:
    explicit ArduinoDigitalInput(uint8_t pin, uint32_t type = INPUT) {
        m_pin = pin;
        m_type = type;
    }

    void setup(DebugStream* debugStream) override {
        pinMode(m_pin, m_type);
    }

    void read() override {
        m_high = digitalRead(m_pin) == HIGH;
    }

private:
    uint8_t m_pin;
    uint32_t m_type;
};

#endif //ADIGITALPIN_H
