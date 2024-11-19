#include "MS8607Sensor.h"
#include "ConstantsUnits.h"

/**
 * @brief Initialize the sensor
 * @details Enabling any peripherals, confirm sensor is talking, set configuration registers on the sensor
 */
void MS8607Sensor::setup() {
    Serial.println("Starting barometric sensor");
    while(!m_barometricSensor.begin()) {
        Serial.println("Barometric sensor failed to load. Waiting 100 ms");
        delay(100);
    };
}

/**
 * @brief Read data from the sensor
 * @details Read in one reading from the sensor, and convert the data to usefully units/numbers.
 * Current implemented functionality are: pressure (Pa), temperature (K), humidity (%rh), and altitude (m).
 */
void MS8607Sensor::read() {
//    return;
    // it sounds like the barometric sensor reports pressure in terms of millibars, but we want to convert to Pascals
    m_pressurePa = m_barometricSensor.getPressure() * Units::MBAR_TO_PA;  // reported in mbar, converting to pascals
    m_temperatureK = m_barometricSensor.getTemperature() + Units::C_TO_K;    // reported in C
    m_humidityPercent = m_barometricSensor.getHumidity();     // reported in terms of relative humidity (%rh)

    calculateAltitude();
    calculateAbsoluteHumidity();

    // printing for testing
//    Serial.print("Pressure (Pa): ");
//    Serial.println(getPressurePa());
//    Serial.print("Temperature (K): ");
//    Serial.println(getTemperatureK());
//    Serial.print("Relative Humidity (rh): ");
//    Serial.println(getHumidityPercent());
//    Serial.print("Absolute Humidity (g/m^3): ");
//    Serial.println(getAbsoluteHumidity(), 10);
//    Serial.print("Altitude (m): ");
//    Serial.println(getAltitudeM());
//    Serial.println();
}