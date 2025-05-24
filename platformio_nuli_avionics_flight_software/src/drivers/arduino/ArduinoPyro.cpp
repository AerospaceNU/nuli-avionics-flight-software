#include <Avionics.h>
#include "../../core/generic_hardware/GenericSensor.h"
#include "ArduinoPyro.h"
#include "../../core/generic_hardware/Pyro.h"
#include "Arduino.h"


void ArduinoPyro::setup() {
    pinMode(m_firePin, OUTPUT);
    disable();
}

void ArduinoPyro::read() {
    m_continuityValue = analogRead(m_continuityPin);
    if (m_continuityThreshold < 0) {
        m_hasContinuity = m_continuityValue <= -m_continuityThreshold;
    } else {
        m_hasContinuity = m_continuityValue >= m_continuityThreshold;
    }
}

bool ArduinoPyro::hasContinuity() const {
    return m_hasContinuity;
}

void ArduinoPyro::fire() const {
    digitalWrite(m_firePin, HIGH);
}

void ArduinoPyro::disable() const {
    digitalWrite(m_firePin, LOW);
}

int ArduinoPyro::rawAdcValue() const {
    return m_continuityValue;
}