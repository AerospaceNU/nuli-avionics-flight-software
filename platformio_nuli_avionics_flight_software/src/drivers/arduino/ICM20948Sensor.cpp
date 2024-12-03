#include "ICM20948Sensor.h"
#include "ConstantsUnits.h"

ICM20948Sensor::ICM20948Sensor(uint8_t chipSelectPin) : m_chipSelectPin(chipSelectPin) {}

void ICM20948Sensor::setSpiClass(SPIClass* spiClass) {
    m_spiClass = spiClass;
}

void ICM20948Sensor::setup() {
    Serial.println("Starting ICM20948 sensors");
    if (m_spiClass == nullptr) {
        m_sparkfunIcm20948.begin(m_chipSelectPin);
    } else {
        m_sparkfunIcm20948.begin(m_chipSelectPin, *m_spiClass);
    }
}

void ICM20948Sensor::read() {
    if (m_sparkfunIcm20948.dataReady()) {
        // Actually get the data from the sensor
        m_sparkfunIcm20948.getAGMT();


        double temperatureK = m_sparkfunIcm20948.temp() + Units::C_TO_K;

        // provided in milli g --> converting to m/s^2
        Vector3D_s accelerationsMSS = {
                m_sparkfunIcm20948.accX() * Units::MILLI_TO_BASE * Constants::G_EARTH_MSS,
                m_sparkfunIcm20948.accY() * Units::MILLI_TO_BASE * Constants::G_EARTH_MSS,
                m_sparkfunIcm20948.accZ() * Units::MILLI_TO_BASE * Constants::G_EARTH_MSS,
        };

        // provided in degrees per second --> converting to radians per second (* 0.017453)
        Vector3D_s velocitiesRadS = {
                m_sparkfunIcm20948.gyrX() * Units::DEGS_TO_RAD,
                m_sparkfunIcm20948.gyrY() * Units::DEGS_TO_RAD,
                m_sparkfunIcm20948.gyrZ() * Units::DEGS_TO_RAD,
        };

        // provided in micro tesla --> converting to teslas ( /10^6)
        Vector3D_s magneticFieldTesla = {
                m_sparkfunIcm20948.magX() * Units::MICRO_TO_BASE,
                m_sparkfunIcm20948.magY() * Units::MICRO_TO_BASE,
                m_sparkfunIcm20948.magZ() * Units::MICRO_TO_BASE,
        };

        m_accelerometer.inject(accelerationsMSS, temperatureK);
        m_gyroscope.inject(velocitiesRadS, temperatureK);
        m_magnetometer.inject(magneticFieldTesla, temperatureK);
    }

    // Verifying through Print Values
//    Serial.println();
//    Serial.print("Temperature: ");
//    Serial.println(m_accelerometer.getTemperatureK());
//
//    Serial.print("Acceleration (g) X: ");
//    Serial.print(m_accelerometer.getAccelerationsMSS().x);
//    Serial.print(", Y: ");
//    Serial.print(m_accelerometer.getAccelerationsMSS().y);
//    Serial.print(", Z: ");
//    Serial.println(m_accelerometer.getAccelerationsMSS().z);
//
//    Serial.print("Velocities (radians/second) X: ");
//    Serial.print(m_gyroscope.getVelocitiesRadS().x);
//    Serial.print(", Y: ");
//    Serial.print(m_gyroscope.getVelocitiesRadS().y);
//    Serial.print(", Z: ");
//    Serial.println(m_gyroscope.getVelocitiesRadS().z);
//
//    Serial.print("Magnetic Field (teslas) X: ");
//    Serial.print(m_magnetometer.getMagneticFieldTesla().x, 8);
//    Serial.print(", Y: ");
//    Serial.print(m_magnetometer.getMagneticFieldTesla().y, 8);
//    Serial.print(", Z: ");
//    Serial.println(m_magnetometer.getMagneticFieldTesla().z, 8);
//    Serial.println();
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

