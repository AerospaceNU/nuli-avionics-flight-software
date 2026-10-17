#include "ICM20602Sensor.h"
#include "ConstantsUnits.h"
#include <Arduino.h>

#include <Wire.h>

#define ICM20602_ADDR 0x69
#define WHO_AM_I_REG 0x75
#define PWR_MGMT_1   0x6B
#define ACCEL_CONFIG 0x1C
#define GYRO_CONFIG  0x1B
#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H  0x43
#define CONFIG           0x1A  // Gyro DLPF config
#define ACCEL_CONFIG2    0x1D  // Accel DLPF config
#define SMPLRT_DIV        0x19

float accelScaleFactor = 2048.0; // Scale factor for ±16g (LSB per g)
float gyroScaleFactor = 16.4; // Scale factor for ±2000 dps (LSB per dps)

void writeRegister(uint8_t address, uint8_t reg, uint8_t data) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}

uint8_t readRegister(uint8_t address, uint8_t reg) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(address, (uint8_t)1);
    return Wire.read();
}

ICM20602Sensor::ICM20602Sensor(const Vector3DTransform* transform): m_accelerometer(transform), m_gyroscope(transform) {}


void ICM20602Sensor::readAccelAndGyroBatch() {
    Wire.beginTransmission(ICM20602_ADDR);
    Wire.write(ACCEL_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom(ICM20602_ADDR, (uint8_t)14); // 6 accel + 2 temp + 6 gyro = 14 bytes

    uint8_t buf[14];
    for (int i = 0; i < 14; i++) {
        buf[i] = Wire.read();
    }

    accelData[0] = (int16_t)((buf[0] << 8) | buf[1]);
    accelData[1] = (int16_t)((buf[2] << 8) | buf[3]);
    accelData[2] = (int16_t)((buf[4] << 8) | buf[5]);

    tempRaw = (int16_t)((buf[6] << 8) | buf[7]);

    gyroData[0] = (int16_t)((buf[8] << 8) | buf[9]);
    gyroData[1] = (int16_t)((buf[10] << 8) | buf[11]);
    gyroData[2] = (int16_t)((buf[12] << 8) | buf[13]);
}


void initICM20602() {
    writeRegister(ICM20602_ADDR, PWR_MGMT_1, 0x00); // Wake up
    delay(100);

    writeRegister(ICM20602_ADDR, SMPLRT_DIV, 0x00); // Max sample rate

    // Enable DLPF for gyroscope (FCHOICE_B = 0), DLPF_CFG = 0 → ~250 Hz bandwidth
    writeRegister(ICM20602_ADDR, CONFIG, 0x00);

    // Enable DLPF for accelerometer (ACCEL_FCHOICE_B = 0), A_DLPF_CFG = 0 → ~218.1 Hz
    writeRegister(ICM20602_ADDR, ACCEL_CONFIG2, 0x00);

    // Set accel range to ±16g
    uint8_t accelConfig = readRegister(ICM20602_ADDR, ACCEL_CONFIG);
    accelConfig = (accelConfig & ~0x18) | 0x18;
    writeRegister(ICM20602_ADDR, ACCEL_CONFIG, accelConfig);
    accelScaleFactor = 2048.0;

    // Set gyro range to ±2000 dps
    uint8_t gyroConfig = readRegister(ICM20602_ADDR, GYRO_CONFIG);
    gyroConfig = (gyroConfig & ~0x18) | 0x18;
    writeRegister(ICM20602_ADDR, GYRO_CONFIG, gyroConfig);
    gyroScaleFactor = 16.4;
}

void ICM20602Sensor::setup(DebugStream* debugStream) {
    Wire.begin();
    Wire.setClock(400000);

    initICM20602();

    const uint8_t whoAmI = readRegister(ICM20602_ADDR, WHO_AM_I_REG);
    if (whoAmI == 0x12) { // Expected device ID for ICM-20602
        debugStream->message("ICM20602 initialized");
    } else {
        debugStream->error("ICM20602 initialization failed, unexpected device ID: %d", whoAmI);
    }
}

void ICM20602Sensor::read() {
    readAccelAndGyroBatch();

    float temperature_K = (((float)tempRaw / 326.8) + 25.0) + Units::C_TO_K;

    const float accelX_g = (float)accelData[0] / accelScaleFactor;
    const float accelY_g = (float)accelData[1] / accelScaleFactor;
    const float accelZ_g = (float)accelData[2] / accelScaleFactor;

    const float gyroX_dps = (float)gyroData[0] / gyroScaleFactor;
    const float gyroY_dps = (float)gyroData[1] / gyroScaleFactor;
    const float gyroZ_dps = (float)gyroData[2] / gyroScaleFactor;

    const Vector3D_s accelerationsMSS = {
            float(accelX_g * Constants::G_EARTH_MSS),
            float(accelY_g * Constants::G_EARTH_MSS),
            float(accelZ_g * Constants::G_EARTH_MSS),
        };

    const Vector3D_s velocitiesRadS = {
            float(gyroX_dps * Units::DEGS_TO_RAD),
            float(gyroY_dps * Units::DEGS_TO_RAD),
            float(gyroZ_dps * Units::DEGS_TO_RAD),
        };

    m_accelerometer.inject(accelerationsMSS, temperature_K);
    m_gyroscope.inject(velocitiesRadS, temperature_K);
}

Accelerometer* ICM20602Sensor::getAccelerometer() {
    return &m_accelerometer;
}

Gyroscope* ICM20602Sensor::getGyroscope() {
    return &m_gyroscope;
}
