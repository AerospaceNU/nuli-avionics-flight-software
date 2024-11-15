//
// Created by chris on 10/21/2024.
//
#include "MS8607driver.h"

/**
 * @brief Initialize the sensor
 * @details Enabling any peripherals, confirm sensor is talking, set configuration registers on the sensor
 */
void MS8607driver::setup() {
    Wire.begin();
    Serial.println("Starting barometric sensor");
    while(!barometricSensor.begin()) {
        Serial.println("Barometric sensor failed to load. Waiting 100 ms");
        delay(100);
    };
}

/**
 * @brief Read data from the sensor
 * @details Read in one reading from the sensor, and convert the data to usefully units/numbers.
 * Current implemented functionality are: pressure (Pa), temperature (K), humidity (%rh), and altitude (m).
 */
void MS8607driver::read() {
    // it sounds like the barometric sensor reports pressure in terms of millibars, but we want to convert to Pascals
    m_pressurePa = barometricSensor.getPressure() * 100.0;  // reported in mbar, converting to pascals  @TODO convert to global constant
    m_temperatureK = barometricSensor.getTemperature() + 273.15;    // reported in C @TODO convert to global constant
    m_humidityPercent = barometricSensor.getHumidity();     // reported in terms of relative humidity (%rh)

    calculateAltitude();
    calculateAbsoluteHumidity();

    // printing for testing
    Serial.print("Pressure (Pa): ");
    Serial.println(getPressurePa());
    Serial.print("Temperature (K): ");
    Serial.println(getTemperatureK());
    Serial.print("Relative Humidity (rh): ");
    Serial.println(getHumidityPercent());
    Serial.print("Absolute Humidity (g/m^3): ");
    Serial.println(getAbsoluteHumidity(), 10);
    Serial.print("Altitude (m): ");
    Serial.println(getAltitudeM());
    Serial.println();
}