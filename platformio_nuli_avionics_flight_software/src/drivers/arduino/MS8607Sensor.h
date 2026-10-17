#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS8607SENSOR_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS8607SENSOR_H

#include "core/generic_hardware/GenericHardware.h"
#include <Wire.h>

// The SparkFun MS8607 library expects SerialUSB
#define SerialUSB Serial
#include <SparkFun_PHT_MS8607_Arduino_Library.h>

/** @brief MS8607 barometer via the SparkFun PHT library (datasheet: https://cdn.sparkfun.com/assets/b/a/f/b/f/MS8607_Datasheet.pdf) */
class MS8607Sensor : public Barometer {
public:
    void setup() override;

    void read() override;

private:
    MS8607 m_barometricSensor;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS8607SENSOR_H
