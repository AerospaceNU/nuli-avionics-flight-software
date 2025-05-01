#include "ArduinoVoltageSensor.h"
#include "Arduino.h"

ArduinoVoltageSensor::ArduinoVoltageSensor(uint8_t pin, double scaleFactor) : VoltageSensor(scaleFactor), m_pin(pin) {}


void ArduinoVoltageSensor::read() {
    m_rawVoltage = analogRead(m_pin);
    scale();
}
