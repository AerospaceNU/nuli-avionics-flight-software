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

    ICM_20948_fss_t fullScaleSettings;
    fullScaleSettings.a = gpm16;
    m_sparkfunIcm20948.setFullScale(ICM_20948_Internal_Acc, fullScaleSettings);
}

void ICM20948Sensor::read() {
    if (m_sparkfunIcm20948.dataReady()) {
        m_sparkfunIcm20948.getAGMT();


        double temperatureK = m_sparkfunIcm20948.temp() + Units::C_TO_K;

        Vector3D_s accelerationsMSS = {
                m_sparkfunIcm20948.accX() * Units::MILLI_TO_BASE * Constants::G_EARTH_MSS,
                m_sparkfunIcm20948.accY() * Units::MILLI_TO_BASE * Constants::G_EARTH_MSS,
                m_sparkfunIcm20948.accZ() * Units::MILLI_TO_BASE * Constants::G_EARTH_MSS,
        };

        Vector3D_s velocitiesRadS = {
                m_sparkfunIcm20948.gyrX() * Units::DEGS_TO_RAD,
                m_sparkfunIcm20948.gyrY() * Units::DEGS_TO_RAD,
                m_sparkfunIcm20948.gyrZ() * Units::DEGS_TO_RAD,
        };

        Vector3D_s magneticFieldTesla = {
                m_sparkfunIcm20948.magX() * Units::MICRO_TO_BASE,
                m_sparkfunIcm20948.magY() * Units::MICRO_TO_BASE,
                m_sparkfunIcm20948.magZ() * Units::MICRO_TO_BASE,
        };

        m_accelerometer.inject(accelerationsMSS, temperatureK);
        m_gyroscope.inject(velocitiesRadS, temperatureK);
        m_magnetometer.inject(magneticFieldTesla, temperatureK);
    }
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

