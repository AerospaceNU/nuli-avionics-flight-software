#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20948SENSOR_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20948SENSOR_H

#include <Avionics.h>
#include "core/generic_hardware/GenericHardware.h"
#include <ICM_20948.h>

/** @brief ICM20948 driver wrapping the SparkFun ICM_20948 library (SPI) */
class ICM20948Sensor final : public GenericSensor {
public:
    explicit ICM20948Sensor(uint8_t chipSelectPin);

    void setSpiClass(SPIClass* spiClass);

    void setup();

    void read() final;

    Accelerometer* getAccelerometer();

    Gyroscope* getGyroscope();

    Magnetometer* getMagnetometer();

private:
    const uint8_t m_chipSelectPin;
    SPIClass* m_spiClass = nullptr;
    ICM_20948_SPI m_sparkfunIcm20948;
    Accelerometer m_accelerometer;
    Gyroscope m_gyroscope;
    Magnetometer m_magnetometer;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20948SENSOR_H
