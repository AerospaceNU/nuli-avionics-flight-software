#include <Avionics.h>
#include "../../core/generic_hardware/GenericSensor.h"
#include "ArduinoPyro.h"
#include "../../core/generic_hardware/Pyro.h"
#include "Arduino.h"


void ArduinoPyro::setup(DebugStream* debugStream) {
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

    if (m_timedFireAlarm.isInitialized() && m_timedFireAlarm.isAlarmFinished(millis())) {
        disable();
        m_timedFireAlarm.reset();
    }
}

bool ArduinoPyro::hasContinuity() const {
    return m_hasContinuity;
}

void ArduinoPyro::fire() {
    m_isFired = true;
    m_timedFireAlarm.reset();
    digitalWrite(m_firePin, HIGH);
}

void ArduinoPyro::disable() {
    m_isFired = false;
    digitalWrite(m_firePin, LOW);
}

int ArduinoPyro::rawAdcValue() const {
    return m_continuityValue;
}

bool ArduinoPyro::isFired() const {
    return m_isFired;
}

void ArduinoPyro::fireFor(const uint32_t timeMs) {
    fire();
    m_timedFireAlarm.startAlarm(millis(), timeMs);
}
