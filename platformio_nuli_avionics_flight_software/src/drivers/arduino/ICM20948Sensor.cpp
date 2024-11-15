#include "ICM20948Sensor.h"

ICM20948Sensor::ICM20948Sensor(uint8_t chipSelectPin) : m_chipSelectPin(chipSelectPin) {}

void ICM20948Sensor::setSpiClass(SPIClass* spiClass) {
    m_spiClass = spiClass;
}

void ICM20948Sensor::setup() {
    Serial.println("Starting ICM20948 sensors");
    if (m_spiClass == nullptr) {
        sparkfunIcm20948.begin(m_chipSelectPin);
    } else {
        sparkfunIcm20948.begin(m_chipSelectPin, *m_spiClass);
    }
}

void ICM20948Sensor::read() {
    Serial.println("Running");
    if (sparkfunIcm20948.dataReady()) {
        // Actually get the data from the sensor
        sparkfunIcm20948.getAGMT();

        // @todo convert units

        double temperatureK = sparkfunIcm20948.temp() + 273.15; //converting to kelvin @todo create global constant

        // provided in milli g --> converting to g  ( / 10^3)
        Vector3D_s accelerationsMSS = {
                sparkfunIcm20948.accX() / 1000.0,
                sparkfunIcm20948.accY() / 1000.0,
                sparkfunIcm20948.accZ() / 1000.0,
        };

        // provided in degrees per second --> converting to radians per second (* 0.017453)
        Vector3D_s velocitiesRadS = {
                sparkfunIcm20948.gyrX() * 0.017453,
                sparkfunIcm20948.gyrY() * 0.017453,
                sparkfunIcm20948.gyrZ() * 0.017453,
        };

        // provided in micro tesla --> converting to teslas ( /10^6)
        Vector3D_s magneticFieldGauss = {
                sparkfunIcm20948.magX() / 1000000.0,
                sparkfunIcm20948.magY() / 1000000.0,
                sparkfunIcm20948.magZ() / 1000000.0,
        };

        m_accelerometer.inject(accelerationsMSS, temperatureK);
        m_gyroscope.inject(velocitiesRadS, temperatureK);
        m_magnetometer.inject(magneticFieldGauss, temperatureK);
    }

    // Verifying through Print Values
    Serial.println();
    Serial.print("Temperature: ");
    Serial.println(m_accelerometer.getTemperatureK());

    Serial.print("Acceleration (g) X: ");
    Serial.print(m_accelerometer.getAccelerationsMSS().x);
    Serial.print(", Y: ");
    Serial.print(m_accelerometer.getAccelerationsMSS().y);
    Serial.print(", Z: ");
    Serial.println(m_accelerometer.getAccelerationsMSS().z);

    Serial.print("Velocities (radians/second) X: ");
    Serial.print(m_gyroscope.getVelocitiesRadS().x);
    Serial.print(", Y: ");
    Serial.print(m_gyroscope.getVelocitiesRadS().y);
    Serial.print(", Z: ");
    Serial.println(m_gyroscope.getVelocitiesRadS().z);

    Serial.print("Magnetic Field (teslas) X: ");
    Serial.print(m_magnetometer.getMagneticFieldGauss().x, 8);
    Serial.print(", Y: ");
    Serial.print(m_magnetometer.getMagneticFieldGauss().y, 8);
    Serial.print(", Z: ");
    Serial.println(m_magnetometer.getMagneticFieldGauss().z, 8);
    Serial.println();
}

Accelerometer* ICM20948Sensor::getAccelerometer() {
    return &m_accelerometer;
}

Gyroscope* ICM20948Sensor::getGyroscope() {
    return &m_gyroscope;
}

Magnetometer* ICM20948Sensor::getMagnetometer() {
    return &m_magnetometer;
}

