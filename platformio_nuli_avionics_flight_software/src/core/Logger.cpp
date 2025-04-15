#include "Logger.h"

#include <Arduino.h>

typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    double baroPressurePa;
    double baroTemperatureK;
    double baroAltitudeM;
    double ax;
    double ay;
    double az;
    double vx;
    double vy;
    double vz;
    double batt;
} LogData;

static LogData logData;

static uint8_t logDataBuffer[sizeof(LogData)];

void Logger::setup(HardwareAbstraction* hardware, Configuration* configuration) {
    m_hardware = hardware;
    m_configuration = configuration;
    m_logWriteAddress = 0;

    FlashMemory* flash = m_hardware->getFlashMemory(0);

    bool foundEmptyPacket = false;

    while (true) {
        // Serial.print("Checking ");
        // Serial.println(m_logWriteAddress);
        // Serial.println(sizeof(logDataBuffer));
        flash->read(m_logWriteAddress, logDataBuffer, sizeof(logDataBuffer));
        foundEmptyPacket = true;
        for (unsigned int i = 0; i < sizeof(logDataBuffer); ++i) {
            if (logDataBuffer[i] != 0xFF) {
                foundEmptyPacket = false;
                break;
            }
        }
        if (foundEmptyPacket) {
            break;
        }
        m_logWriteAddress += sizeof(logDataBuffer);
    }
    Serial.print("Starting logging at ");
    Serial.println(m_logWriteAddress);
}

void Logger::log(float batt) {
    FlashMemory* flash = m_hardware->getFlashMemory(0);


    logData.baroAltitudeM = m_hardware->getBarometer(0)->getAltitudeM();
    logData.baroPressurePa = m_hardware->getBarometer(0)->getPressurePa();
    logData.baroTemperatureK = m_hardware->getBarometer(0)->getTemperatureK();
    logData.timestamp = m_hardware->getLoopTimestampMs();


    Vector3D_s accelerationsMss = m_hardware->getAccelerometer(0)->getAccelerationsMSS();
    Vector3D_s velocitiesRadS = m_hardware->getGyroscope(0)->getVelocitiesRadS();

    logData.ax = accelerationsMss.x;
    logData.ay = accelerationsMss.y;
    logData.az = accelerationsMss.z;
    logData.vx = velocitiesRadS.x;
    logData.vy = velocitiesRadS.y;
    logData.vz = velocitiesRadS.z;
    logData.batt = batt;
    // Serial.println(logData.timestamp);
    // Serial.println(sizeof(logData));

    flash->write(m_logWriteAddress, (uint8_t*) &logData, sizeof(logData), true);
    m_logWriteAddress += sizeof(logData);
}

uint32_t Logger::offloadData(uint32_t readAddress, uint8_t* buffer, const uint32_t length) {
    FlashMemory* flash = m_hardware->getFlashMemory(0);
    uint32_t readLength = min(length, max((unsigned int) 0, m_logWriteAddress - readAddress));
    flash->read(readAddress, buffer, length);
    return readLength;
}

void Logger::erase() {
    FlashMemory* flash = m_hardware->getFlashMemory(0);
    flash->eraseAll();
    m_logWriteAddress = 0;
}