#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20948_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20948_H

#include <Avionics.h>
#include <GenericSensor.h>
#include <Accelerometer.h>
#include <Gyroscope.h>
#include <Magnetometer.h>
#include <ICM_20948.h>

class ICM20948 final : public GenericSensor {
public:
    explicit ICM20948(uint8_t chipSelectPin);

    void setSpiClass(SPIClass* spiClass);

    void setup() final;

    void read() final;

    Accelerometer* getAccelerometer();

    Gyroscope* getGyroscope();

    Magnetometer* getMagnetometer();

private:
    const uint8_t m_chipSelectPin;
    SPIClass* m_spiClass = nullptr;

    ICM_20948_SPI sparkfunIcm20948;
    Accelerometer m_accelerometer;
    Gyroscope m_gyroscope;
    Magnetometer m_magnetometer;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20948_H
