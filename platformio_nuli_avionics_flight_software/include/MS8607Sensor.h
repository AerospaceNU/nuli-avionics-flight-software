#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS8607SENSOR_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS8607SENSOR_H

#include "Barometer.h"
#include <Wire.h>
#define SerialUSB Serial
#include <SparkFun_PHT_MS8607_Arduino_Library.h>    // library for MS8607

/**
 * @class MS8607Sensor
 * @brief An implementation of Barometer for the MS8607 barometer.
 * @details Uses the SparkFun PHT MS8607 Arduino library and implements an abstraction layer for our format.
 * The datasheet can be found here: https://cdn.sparkfun.com/assets/b/a/f/b/f/MS8607_Datasheet.pdf
 *
 */
class MS8607Sensor : public Barometer {
public:
    /**
     * @brief Initialize the sensor
     * @details Enabling any peripherals, confirm sensor is talking, set configuration registers on the sensor
     */
    void setup() override;

    /**
     * @brief Read data from the sensor
     * @details Read in one reading from the sensor, and convert the data to usefully units/numbers.
     * Currently is allowed to block the loop to wait for data from the sensor for a few ms.
     */
    void read() override;

private:
    MS8607 barometricSensor;    // the library that handles all the messy stuff for the MS8607 sensor
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS8607SENSOR_H
