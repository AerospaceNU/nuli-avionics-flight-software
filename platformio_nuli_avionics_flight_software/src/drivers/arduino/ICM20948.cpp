#include "ICM20948.h"

ICM20948::ICM20948(uint8_t chipSelectPin) : m_chipSelectPin(chipSelectPin) {}

void ICM20948::setSpiClass(SPIClass* spiClass) {
    m_spiClass = spiClass;
}

void ICM20948::setup() {
    if (m_spiClass == nullptr) {
        sparkfunIcm20948.begin(m_chipSelectPin);
    } else {
        sparkfunIcm20948.begin(m_chipSelectPin, *m_spiClass);
    }
}

void ICM20948::read() { 
    if (sparkfunIcm20948.dataReady()) {
        // Actually get the data from the sensor
        sparkfunIcm20948.getAGMT();

        // @todo convert units

        double temperatureK = sparkfunIcm20948.temp();

        Vector3D_s accelerationsMSS = {
                sparkfunIcm20948.accX(),
                sparkfunIcm20948.accY(),
                sparkfunIcm20948.accZ(),
        };

        Vector3D_s velocitiesRadS = {
                sparkfunIcm20948.gyrX(),
                sparkfunIcm20948.gyrY(),
                sparkfunIcm20948.gyrZ(),
        };

        Vector3D_s magneticFieldGauss = {
                sparkfunIcm20948.magX(),
                sparkfunIcm20948.magY(),
                sparkfunIcm20948.magZ(),
        };

        m_accelerometer.inject(accelerationsMSS, temperatureK);
        m_gyroscope.inject(velocitiesRadS, temperatureK);
        m_magnetometer.inject(magneticFieldGauss, temperatureK);
    }
}

Accelerometer* ICM20948::getAccelerometer() {
    return &m_accelerometer;
}

Gyroscope* ICM20948::getGyroscope() {
    return &m_gyroscope;
}

Magnetometer* ICM20948::getMagnetometer() {
    return &m_magnetometer;
}

