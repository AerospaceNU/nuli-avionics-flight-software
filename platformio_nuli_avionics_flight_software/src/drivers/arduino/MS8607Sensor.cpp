#include "MS8607Sensor.h"
#include "ConstantsUnits.h"

void MS8607Sensor::setup() {
    Serial.println("Starting barometric sensor");
    while(!m_barometricSensor.begin()) {
        Serial.println("Barometric sensor failed to load. Waiting 100 ms");
        delay(100);
    };
}

void MS8607Sensor::read() {
    m_pressurePa = m_barometricSensor.getPressure() * Units::MBAR_TO_PA;
    m_validReading = isPlausiblePressure(m_pressurePa);
    m_temperatureK = m_barometricSensor.getTemperature() + Units::C_TO_K;
    m_humidityPercent = m_barometricSensor.getHumidity();

    calculateAltitude();
    calculateAbsoluteHumidity();
}