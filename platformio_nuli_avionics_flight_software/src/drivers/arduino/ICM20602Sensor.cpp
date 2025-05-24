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
#define CONFIG           0x1A  // Gyro DLPF config
#define ACCEL_CONFIG2    0x1D  // Accel DLPF config
#define SMPLRT_DIV        0x19

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

void ICM20602Sensor::readAccelAndGyroBatch() {
    Wire.beginTransmission(ICM20602_ADDR);
    Wire.write(ACCEL_XOUT_H);      // 0x2D
    Wire.endTransmission(false);
    Wire.requestFrom(ICM20602_ADDR, (uint8_t) 14);  // 6 accel + 2 temp + 6 gyro = 14 bytes

    uint8_t buf[14];
    for (int i = 0; i < 14; i++) {
        buf[i] = Wire.read();
    }

    // Parse accel
    accelData[0] = (int16_t) ((buf[0] << 8) | buf[1]);
    accelData[1] = (int16_t) ((buf[2] << 8) | buf[3]);
    accelData[2] = (int16_t) ((buf[4] << 8) | buf[5]);

    // Parse temperature (optional)
    tempRaw = (int16_t) ((buf[6] << 8) | buf[7]);

    // Parse gyro
    gyroData[0] = (int16_t) ((buf[8] << 8) | buf[9]);
    gyroData[1] = (int16_t) ((buf[10] << 8) | buf[11]);
    gyroData[2] = (int16_t) ((buf[12] << 8) | buf[13]);

}


void initICM20602() {
    writeRegister(ICM20602_ADDR, PWR_MGMT_1, 0x00);  // Wake up
    delay(100);

    // Set SMPLRT_DIV to 0 -> sample rate = internal rate / (1 + SMPLRT_DIV)
    writeRegister(ICM20602_ADDR, SMPLRT_DIV, 0x00);  // Max sample rate

    // Enable DLPF for gyroscope (FCHOICE_B = 0), DLPF_CFG = 0 → ~250 Hz bandwidth
    writeRegister(ICM20602_ADDR, CONFIG, 0x00);  // DLPF_CFG = 0

    // Enable DLPF for accelerometer (ACCEL_FCHOICE_B = 0), A_DLPF_CFG = 0 → ~218.1 Hz
    writeRegister(ICM20602_ADDR, ACCEL_CONFIG2, 0x00);  // A_DLPF_CFG = 0

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


ICM20602Sensor::ICM20602Sensor() = default;


void ICM20602Sensor::setup() {
    Serial.println("Starting ICM20602 sensors");
    Wire.begin();
    Wire.setClock(400000);

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
    readAccelAndGyroBatch();

    // @todo is this right? incorporate into rest of code
//    float temperature_K = (((float) tempRaw / 326.8) + 25.0) + Units::C_TO_K;

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


