#include "ICM42605Sensor.h"
#include "ConstantsUnits.h"
#include <Arduino.h>
#include <Wire.h>

/* =========================
 * ICM-42605 Register Map
 * ========================= */

#define ICM42605_ADDR        0x68

/* Bank select */
#define REG_BANK_SEL         0x76

/* -------- Bank 0 -------- */
#define WHO_AM_I_REG         0x75
#define DEVICE_CONFIG        0x11
#define PWR_MGMT0            0x4E

#define TEMP_DATA1           0x1D
#define ACCEL_DATA_X1        0x1F   // Accel + Gyro + Timestamp follow

/* -------- Bank 1 -------- */
#define GYRO_CONFIG0         0x4F
#define ACCEL_CONFIG0        0x50

/* =========================
 * Scale factors
 * ========================= */

static float accelScaleFactor = 2048.0f; // ±16g
static float gyroScaleFactor  = 16.4f;   // ±2000 dps

/* =========================
 * Low-level helpers
 * ========================= */

static void selectBank(uint8_t bank) {
    Wire.beginTransmission(ICM42605_ADDR);
    Wire.write(REG_BANK_SEL);
    Wire.write(bank);
    Wire.endTransmission();
}

static void writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(ICM42605_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

static uint8_t readRegister(uint8_t reg) {
    Wire.beginTransmission(ICM42605_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(ICM42605_ADDR, (uint8_t)1);
    return Wire.read();
}

/* =========================
 * Constructor
 * ========================= */

ICM42605Sensor::ICM42605Sensor(const Vector3DTransform* transform)
    : m_accelerometer(transform),
      m_gyroscope(transform) {}

/* =========================
 * Sensor init
 * ========================= */

static void initICM42605() {
    selectBank(0);

    /* Soft reset */
    writeRegister(DEVICE_CONFIG, 0x01);
    delay(50);

    /* Enable accel + gyro (low-noise mode) */
    writeRegister(PWR_MGMT0, 0b00001111);
    delay(50);

    // Gyro ±2000 dps, ODR = 200 Hz
    writeRegister(GYRO_CONFIG0,  0b00000111);
    // // Accel ±16g, ODR = 200 Hz
    writeRegister(ACCEL_CONFIG0, 0b00000111);
}

/* =========================
 * Setup
 * ========================= */

void ICM42605Sensor::setup(DebugStream* debugStream) {
    Wire.begin();
    Wire.setClock(400000);

    initICM42605();

    selectBank(0);
    const uint8_t whoAmI = readRegister(WHO_AM_I_REG);

    if (whoAmI == 0x42) {
        debugStream->message("ICM42605 initialized");
    } else {
        debugStream->error(
            "ICM42605 init failed, WHO_AM_I = %d",
            whoAmI
        );
    }
}

/* =========================
 * Batch read
 * =========================
 *
 * Burst layout starting at TEMP_DATA1:
 *
 *  0–1  TEMP
 *  2–7  ACCEL XYZ
 *  8–13 GYRO XYZ
 * 14–15 TIMESTAMP (ignored)
 *
 * ========================= */

void ICM42605Sensor::readAccelAndGyroBatch() {
    selectBank(0);

    Wire.beginTransmission(ICM42605_ADDR);
    Wire.write(TEMP_DATA1);
    Wire.endTransmission(false);

    /* Read TEMP + ACCEL + GYRO + TIMESTAMP */
    Wire.requestFrom(ICM42605_ADDR, (uint8_t)16);

    uint8_t buf[16];
    for (int i = 0; i < 16; i++) {
        buf[i] = Wire.read();
    }

    /* Temperature */
    tempRaw = (int16_t)((buf[0] << 8) | buf[1]);

    /* Accel */
    accelData[0] = (int16_t)((buf[2] << 8) | buf[3]);
    accelData[1] = (int16_t)((buf[4] << 8) | buf[5]);
    accelData[2] = (int16_t)((buf[6] << 8) | buf[7]);

    /* Gyro */
    gyroData[0]  = (int16_t)((buf[8]  << 8) | buf[9]);
    gyroData[1]  = (int16_t)((buf[10] << 8) | buf[11]);
    gyroData[2]  = (int16_t)((buf[12] << 8) | buf[13]);

    /* buf[14–15] = TMST_FSYNCH/L → intentionally ignored */
}

/* =========================
 * Read + inject
 * ========================= */

void ICM42605Sensor::read() {
    readAccelAndGyroBatch();

    const float temperature_K =
        (((float)tempRaw / 132.48f) + 25.0f) + Units::C_TO_K;

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

/* =========================
 * Accessors
 * ========================= */

Accelerometer* ICM42605Sensor::getAccelerometer() {
    return &m_accelerometer;
}

Gyroscope* ICM42605Sensor::getGyroscope() {
    return &m_gyroscope;
}
