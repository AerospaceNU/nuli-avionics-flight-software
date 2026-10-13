#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOSERVO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOSERVO_H

#include "Avionics.h"
#include <Arduino.h>
#include <Servo.h>
#include "core/generic_hardware/GenericHardware.h"

/**
 * @class ArduinoServo
 * @brief A servo driven between two fixed angles.
 * @details Both angles are fixed at construction. setup() attaches the servo and drives it to
 * position 1, so the servo holds a known angle from boot onward. The SAMD Servo library pulses the
 * pin with digitalWrite() from a timer ISR, so any digital pin works - it need not be PWM capable.
 */
class ArduinoServo final : public GenericAvionicsHardware {
public:
    /**
     * @param pin Pin wired to the servo's signal line.
     * @param position1Deg Angle, in degrees, held from boot until moveToPosition2().
     * @param position2Deg Angle, in degrees, moved to by moveToPosition2().
     */
    ArduinoServo(const int8_t pin, const uint8_t position1Deg, const uint8_t position2Deg)
        : m_pin(pin), m_position1Deg(position1Deg), m_position2Deg(position2Deg) {}

    void setup(DebugStream* debugStream, WatchdogTimer* watchdog) override {
        m_servo.attach(m_pin);
        moveToPosition1();
    }

    void moveToPosition1() {
        m_servo.write(m_position1Deg);
        m_atPosition2 = false;
    }

    void moveToPosition2() {
        m_servo.write(m_position2Deg);
        m_atPosition2 = true;
    }

    /// True once moveToPosition2() has run, so a caller can fire the move exactly once.
    bool isAtPosition2() const {
        return m_atPosition2;
    }

private:
    Servo m_servo;
    int8_t m_pin;
    uint8_t m_position1Deg;
    uint8_t m_position2Deg;
    bool m_atPosition2 = false;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOSERVO_H
