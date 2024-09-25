#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_PYRO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_PYRO_H

#include <Avionics.h>
#include <GenericSensor.h>

class Pyro {
public:
    Pyro(uint8_t firePin, uint8_t continuityPin, int32_t continuityThreshold) : m_firePin(firePin), m_continuityPin(continuityPin),
                                                                                m_continuityThreshold(continuityThreshold) {
        if (m_continuityThreshold == USE_DIGITAL_CONTINUITY) {
            pinMode(continuityPin, INPUT);
        }
        pinMode(firePin, OUTPUT);
        disable();
    }

    void setup() {};

    void read() {
        if (m_continuityThreshold == USE_DIGITAL_CONTINUITY) {
            m_hasContinuity = digitalRead(m_continuityPin);
        } else {
            m_hasContinuity = analogRead(m_continuityPin) >= uint32_t(m_continuityThreshold);
        }
    }

    bool hasContinuity() const {
        return m_hasContinuity;
    }

    void fire() const {
        digitalWrite(m_firePin, HIGH);
    }

    void disable() const {
        digitalWrite(m_firePin, LOW);
    }

    static constexpr int32_t USE_DIGITAL_CONTINUITY = -1;

private:
    bool m_hasContinuity = false;
    const uint8_t m_firePin;
    const uint8_t m_continuityPin;
    const int32_t m_continuityThreshold;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_PYRO_H
