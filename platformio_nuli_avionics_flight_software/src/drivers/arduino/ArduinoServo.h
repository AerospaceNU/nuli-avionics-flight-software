#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOSERVO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOSERVO_H

#include "Avionics.h"
#include <Arduino.h>
#include <Servo.h>
#include <algorithm>
#include <cmath>
#include "core/generic_hardware/GenericHardware.h"

class ArduinoServo final : public GenericAvionicsHardware {
public:
    ArduinoServo(const int8_t pin, const uint16_t position0Us, const uint16_t position1Us)
        : m_pin(pin), m_position0Us(position0Us), m_position1Us(position1Us) {}

    void setup(DebugStream* debugStream, WatchdogTimer* watchdog) override {
        m_servo.attach(m_pin, std::min(m_position0Us, m_position1Us), std::max(m_position0Us, m_position1Us));
        setPosition(0.0f);
    }

    void setPosition(const float fraction) {
        m_position = std::max(0.0f, std::min(1.0f, fraction));
        m_servo.writeMicroseconds((int)std::lround((float)m_position0Us + m_position * ((float)m_position1Us - (float)m_position0Us)));
    }

    float getPosition() const {
        return m_position;
    }

private:
    Servo m_servo;
    int8_t m_pin;
    uint16_t m_position0Us;
    uint16_t m_position1Us;
    float m_position = 0.0f;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOSERVO_H
