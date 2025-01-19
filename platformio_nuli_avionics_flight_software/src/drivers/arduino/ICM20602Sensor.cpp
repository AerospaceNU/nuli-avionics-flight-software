#include "ICM20602Sensor.h"
#include "ConstantsUnits.h"
#include <Arduino.h>

#include <Wire.h>

#define ICM20602_ADDR 0x69  // Default I2C address of the ICM-20602
#define WHO_AM_I_REG 0x75   // Register to check device ID
#define PWR_MGMT_1   0x6B   // Power management register
#define ACCEL_CONFIG 0x1C   // Accelerometer configuration register
#define GYRO_CONFIG  0x1B   // Gyroscope configuration register
#define ACCEL_XOUT_H 0x3B   // Accelerometer X-axis high byte register
#define GYRO_XOUT_H  0x43   // Gyroscope X-axis high byte register

float accelScaleFactor = 2048.0;  // Scale factor for ±16g (LSB per g)
float gyroScaleFactor = 16.4;     // Scale factor for ±2000 dps (LSB per dps)

void writeRegister(uint8_t address, uint8_t reg, uint8_t data) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}

uint8_t readRegister(uint8_t address, uint8_t reg) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.endTransmission(false);  // Restart condition
    Wire.requestFrom(address, (uint8_t) 1);
    return Wire.read();
}

void readAccelData(int16_t* data) {
    // Read 6 bytes of accelerometer data (X, Y, Z)
    Wire.beginTransmission(ICM20602_ADDR);
    Wire.write(ACCEL_XOUT_H);
    Wire.endTransmission(false);  // Restart condition
    Wire.requestFrom(ICM20602_ADDR, (uint8_t) 6);

    data[0] = (Wire.read() << 8) | Wire.read();  // X-axis
    data[1] = (Wire.read() << 8) | Wire.read();  // Y-axis
    data[2] = (Wire.read() << 8) | Wire.read();  // Z-axis
}

void readGyroData(int16_t* data) {
    // Read 6 bytes of gyroscope data (X, Y, Z)
    Wire.beginTransmission(ICM20602_ADDR);
    Wire.write(GYRO_XOUT_H);
    Wire.endTransmission(false);  // Restart condition
    Wire.requestFrom(ICM20602_ADDR, (uint8_t) 6);

    data[0] = (Wire.read() << 8) | Wire.read();  // X-axis
    data[1] = (Wire.read() << 8) | Wire.read();  // Y-axis
    data[2] = (Wire.read() << 8) | Wire.read();  // Z-axis
}

void initICM20602() {
    writeRegister(ICM20602_ADDR, PWR_MGMT_1, 0x00);  // Wake up the sensor
    delay(100);

    // Set accelerometer range to ±16g
    uint8_t accelConfig = readRegister(ICM20602_ADDR, ACCEL_CONFIG);
    accelConfig = (accelConfig & ~0x18) | 0x18;  // Clear and set bits [4:3] to 0b11 for ±16g
    writeRegister(ICM20602_ADDR, ACCEL_CONFIG, accelConfig);

    // Update accelerometer scale factor
    accelScaleFactor = 2048.0;  // ±16g scale factor

    // Set gyroscope range to ±2000 dps
    uint8_t gyroConfig = readRegister(ICM20602_ADDR, GYRO_CONFIG);
    gyroConfig = (gyroConfig & ~0x18) | 0x18;  // Clear and set bits [4:3] to 0b11 for ±2000 dps
    writeRegister(ICM20602_ADDR, GYRO_CONFIG, gyroConfig);

    // Update gyroscope scale factor
    gyroScaleFactor = 16.4;  // ±2000 dps scale factor
}

ICM20602Sensor::ICM20602Sensor() = default;


void ICM20602Sensor::setup() {
    Serial.println("Starting ICM20602 sensors");
    Wire.begin();

    initICM20602();

    // Verify device ID
    uint8_t whoAmI = readRegister(ICM20602_ADDR, WHO_AM_I_REG);
    if (whoAmI == 0x12) {  // Expected device ID for ICM-20602
        Serial.println("ICM-20602 detected!");
    } else {
        Serial.print("Unexpected device ID: ");
        Serial.println(whoAmI, HEX);
    }
}

void ICM20602Sensor::read() {
    int16_t accelData[3];  // X, Y, Z
    int16_t gyroData[3];   // X, Y, Z
    readAccelData(accelData);
    readGyroData(gyroData);

    // Convert accelerometer data to g
    float accelX_g = (float) accelData[0] / accelScaleFactor;
    float accelY_g = (float) accelData[1] / accelScaleFactor;
    float accelZ_g = (float) accelData[2] / accelScaleFactor;

    // Convert gyroscope data to dps
    float gyroX_dps = (float) gyroData[0] / gyroScaleFactor;
    float gyroY_dps = (float) gyroData[1] / gyroScaleFactor;
    float gyroZ_dps = (float) gyroData[2] / gyroScaleFactor;

    // provided in milli g --> converting to m/s^2
    Vector3D_s accelerationsMSS = {
            accelX_g * Constants::G_EARTH_MSS,
            accelY_g * Constants::G_EARTH_MSS,
            accelZ_g * Constants::G_EARTH_MSS,
    };

    // provided in degrees per second --> converting to radians per second (* 0.017453)
    Vector3D_s velocitiesRadS = {
            gyroX_dps * Units::DEGS_TO_RAD,
            gyroY_dps * Units::DEGS_TO_RAD,
            gyroZ_dps * Units::DEGS_TO_RAD,
    };

    m_accelerometer.inject(accelerationsMSS, 0);
    m_gyroscope.inject(velocitiesRadS, 0);
}

Accelerometer* ICM20602Sensor::getAccelerometer() {
    return &m_accelerometer;
}

Gyroscope* ICM20602Sensor::getGyroscope() {
    return &m_gyroscope;
}


