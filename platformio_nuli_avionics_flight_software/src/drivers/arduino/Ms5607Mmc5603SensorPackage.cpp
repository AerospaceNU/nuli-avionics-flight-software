#include "Ms5607Mmc5603SensorPackage.h"
#include "ConstantsUnits.h"
#include "I2cBusRecovery.h"
#include <Arduino.h>
#include <Wire.h>

/* =========================
 * Generic I2C register helpers (IMUs and magnetometer; the barometer uses command bytes)
 * ========================= */

static void writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

static uint8_t readRegister(uint8_t address, uint8_t reg) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(address, (uint8_t)1);
    return Wire.read();
}

/* =========================
 * ICM-20602 IMU (register map/init copied from ICM20602Sensor.cpp - SillyGoose V1)
 * ========================= */

#define ICM20602_ADDR      0x69
#define ICM20602_WHO_AM_I  0x75
#define ICM20602_PWR_MGMT_1     0x6B
#define ICM20602_ACCEL_CONFIG   0x1C
#define ICM20602_GYRO_CONFIG    0x1B
#define ICM20602_ACCEL_XOUT_H   0x3B
#define ICM20602_CONFIG         0x1A // gyro DLPF config
#define ICM20602_ACCEL_CONFIG2  0x1D // accel DLPF config
#define ICM20602_SMPLRT_DIV     0x19

static constexpr float ICM20602_ACCEL_SCALE_FACTOR = 2048.0f; // LSB/g at +-16g
static constexpr float ICM20602_GYRO_SCALE_FACTOR  = 16.4f;   // LSB/dps at +-2000dps
// Reciprocals avoid soft-float division on SAMD21; gyro result differs from division by <= 2 ULP
static constexpr float ICM20602_ACCEL_SCALE_FACTOR_RECIP = 1.0f / ICM20602_ACCEL_SCALE_FACTOR;
static constexpr float ICM20602_GYRO_SCALE_FACTOR_RECIP  = 1.0f / ICM20602_GYRO_SCALE_FACTOR;
static constexpr float ICM20602_TEMP_SENSITIVITY_RECIP = 1.0f / 326.8f;

void Ms5607Mmc5603SensorPackage::setupImuIcm20602() {
    writeRegister(ICM20602_ADDR, ICM20602_PWR_MGMT_1, 0x00); // wake up
    delay(100);

    // Sample rate = internal rate / (1 + SMPLRT_DIV) - 0x00 selects the max sample rate.
    writeRegister(ICM20602_ADDR, ICM20602_SMPLRT_DIV, 0x00);
    // Gyro DLPF: FCHOICE_B=0, DLPF_CFG=0 -> ~250Hz bandwidth (also gates SMPLRT_DIV taking effect).
    writeRegister(ICM20602_ADDR, ICM20602_CONFIG, 0x00);
    // Accel DLPF: ACCEL_FCHOICE_B=0, A_DLPF_CFG=0 -> ~218.1Hz bandwidth.
    writeRegister(ICM20602_ADDR, ICM20602_ACCEL_CONFIG2, 0x00);

    // +-16g
    uint8_t accelConfig = readRegister(ICM20602_ADDR, ICM20602_ACCEL_CONFIG);
    accelConfig = (accelConfig & ~0x18) | 0x18;
    writeRegister(ICM20602_ADDR, ICM20602_ACCEL_CONFIG, accelConfig);

    // +-2000dps
    uint8_t gyroConfig = readRegister(ICM20602_ADDR, ICM20602_GYRO_CONFIG);
    gyroConfig = (gyroConfig & ~0x18) | 0x18;
    writeRegister(ICM20602_ADDR, ICM20602_GYRO_CONFIG, gyroConfig);

    const uint8_t whoAmI = readRegister(ICM20602_ADDR, ICM20602_WHO_AM_I);
    if (whoAmI == 0x12) {
        m_debugStream->message("ICM20602 initialized");
    } else {
        m_debugStream->error("ICM20602 init failed, WHO_AM_I = %d", whoAmI);
    }
}

bool Ms5607Mmc5603SensorPackage::readAndInjectImuIcm20602() {
    Wire.beginTransmission(ICM20602_ADDR);
    Wire.write(ICM20602_ACCEL_XOUT_H);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(ICM20602_ADDR, (uint8_t)14) != 14) return false; // 6 accel + 2 temp + 6 gyro

    uint8_t buf[14];
    for (unsigned char & i : buf) i = Wire.read();

    m_accelData[0] = (int16_t)((buf[0] << 8) | buf[1]);
    m_accelData[1] = (int16_t)((buf[2] << 8) | buf[3]);
    m_accelData[2] = (int16_t)((buf[4] << 8) | buf[5]);
    m_imuTempRaw = (int16_t)((buf[6] << 8) | buf[7]);
    m_gyroData[0] = (int16_t)((buf[8] << 8) | buf[9]);
    m_gyroData[1] = (int16_t)((buf[10] << 8) | buf[11]);
    m_gyroData[2] = (int16_t)((buf[12] << 8) | buf[13]);

    const float temperatureK = (((float)m_imuTempRaw * ICM20602_TEMP_SENSITIVITY_RECIP) + 25.0f) + Units::C_TO_K;

    const Vector3D_s accelerationsMSS = {
        (float)m_accelData[0] * ICM20602_ACCEL_SCALE_FACTOR_RECIP * (float)Constants::G_EARTH_MSS,
        (float)m_accelData[1] * ICM20602_ACCEL_SCALE_FACTOR_RECIP * (float)Constants::G_EARTH_MSS,
        (float)m_accelData[2] * ICM20602_ACCEL_SCALE_FACTOR_RECIP * (float)Constants::G_EARTH_MSS,
    };
    const Vector3D_s velocitiesRadS = {
        (float)m_gyroData[0] * ICM20602_GYRO_SCALE_FACTOR_RECIP * (float)Units::DEGS_TO_RAD,
        (float)m_gyroData[1] * ICM20602_GYRO_SCALE_FACTOR_RECIP * (float)Units::DEGS_TO_RAD,
        (float)m_gyroData[2] * ICM20602_GYRO_SCALE_FACTOR_RECIP * (float)Units::DEGS_TO_RAD,
    };

    m_accelerometer.inject(accelerationsMSS, temperatureK);
    m_gyroscope.inject(velocitiesRadS, temperatureK);
    return true;
}

/* =========================
 * ICM-42605 IMU (register map/init copied from ICM42605Sensor.cpp - SillyGoose V2, SeriousGoose)
 * ========================= */

#define ICM42605_ADDR        0x68
#define ICM42605_REG_BANK_SEL  0x76
#define ICM42605_WHO_AM_I      0x75
#define ICM42605_DEVICE_CONFIG 0x11
#define ICM42605_PWR_MGMT0     0x4E
#define ICM42605_TEMP_DATA1    0x1D
#define ICM42605_GYRO_CONFIG0  0x4F
#define ICM42605_ACCEL_CONFIG0 0x50

static constexpr float ICM42605_ACCEL_SCALE_FACTOR = 2048.0f; // LSB/g at +-16g
static constexpr float ICM42605_GYRO_SCALE_FACTOR  = 16.4f;   // LSB/dps at +-2000dps
// See ICM20602_*_RECIP above
static constexpr float ICM42605_ACCEL_SCALE_FACTOR_RECIP = 1.0f / ICM42605_ACCEL_SCALE_FACTOR;
static constexpr float ICM42605_GYRO_SCALE_FACTOR_RECIP  = 1.0f / ICM42605_GYRO_SCALE_FACTOR;
static constexpr float ICM42605_TEMP_SENSITIVITY_RECIP = 1.0f / 132.48f;

static void icm42605SelectBank(uint8_t bank) {
    writeRegister(ICM42605_ADDR, ICM42605_REG_BANK_SEL, bank);
}

void Ms5607Mmc5603SensorPackage::setupImuIcm42605() {
    icm42605SelectBank(0);
    writeRegister(ICM42605_ADDR, ICM42605_DEVICE_CONFIG, 0x01); // soft reset
    delay(50);
    writeRegister(ICM42605_ADDR, ICM42605_PWR_MGMT0, 0b00001111); // accel + gyro, low-noise mode
    delay(50);
    // GYRO_CONFIG0/ACCEL_CONFIG0 live in bank 0 on this chip despite the datasheet's section header
    // suggesting otherwise (matches ICM42605Sensor.cpp). Each byte is range+ODR: 0b0000111 = +-16g/+-2000dps, ODR=200Hz.
    writeRegister(ICM42605_ADDR, ICM42605_GYRO_CONFIG0, 0b00000111);
    writeRegister(ICM42605_ADDR, ICM42605_ACCEL_CONFIG0, 0b00000111);

    const uint8_t whoAmI = readRegister(ICM42605_ADDR, ICM42605_WHO_AM_I);
    if (whoAmI == 0x42) {
        m_debugStream->message("ICM42605 initialized");
    } else {
        m_debugStream->error("ICM42605 init failed, WHO_AM_I = %d", whoAmI);
    }
}

bool Ms5607Mmc5603SensorPackage::readAndInjectImuIcm42605() {
    icm42605SelectBank(0);
    Wire.beginTransmission(ICM42605_ADDR);
    Wire.write(ICM42605_TEMP_DATA1);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(ICM42605_ADDR, (uint8_t)16) != 16) return false; // temp(2) + accel(6) + gyro(6) + timestamp(2, ignored)

    uint8_t buf[16];
    for (unsigned char & i : buf) i = Wire.read();

    m_imuTempRaw = (int16_t)((buf[0] << 8) | buf[1]);
    m_accelData[0] = (int16_t)((buf[2] << 8) | buf[3]);
    m_accelData[1] = (int16_t)((buf[4] << 8) | buf[5]);
    m_accelData[2] = (int16_t)((buf[6] << 8) | buf[7]);
    m_gyroData[0] = (int16_t)((buf[8] << 8) | buf[9]);
    m_gyroData[1] = (int16_t)((buf[10] << 8) | buf[11]);
    m_gyroData[2] = (int16_t)((buf[12] << 8) | buf[13]);

    const float temperatureK = (((float)m_imuTempRaw * ICM42605_TEMP_SENSITIVITY_RECIP) + 25.0f) + Units::C_TO_K;

    const Vector3D_s accelerationsMSS = {
        (float)m_accelData[0] * ICM42605_ACCEL_SCALE_FACTOR_RECIP * (float)Constants::G_EARTH_MSS,
        (float)m_accelData[1] * ICM42605_ACCEL_SCALE_FACTOR_RECIP * (float)Constants::G_EARTH_MSS,
        (float)m_accelData[2] * ICM42605_ACCEL_SCALE_FACTOR_RECIP * (float)Constants::G_EARTH_MSS,
    };
    const Vector3D_s velocitiesRadS = {
        (float)m_gyroData[0] * ICM42605_GYRO_SCALE_FACTOR_RECIP * (float)Units::DEGS_TO_RAD,
        (float)m_gyroData[1] * ICM42605_GYRO_SCALE_FACTOR_RECIP * (float)Units::DEGS_TO_RAD,
        (float)m_gyroData[2] * ICM42605_GYRO_SCALE_FACTOR_RECIP * (float)Units::DEGS_TO_RAD,
    };

    m_accelerometer.inject(accelerationsMSS, temperatureK);
    m_gyroscope.inject(velocitiesRadS, temperatureK);
    return true;
}

/* =========================
 * IMU dispatch
 * ========================= */

void Ms5607Mmc5603SensorPackage::setupImu() {
    if (m_imuType == ImuType::ICM20602) {
        setupImuIcm20602();
    } else {
        setupImuIcm42605();
    }
}

bool Ms5607Mmc5603SensorPackage::readAndInjectImu() {
    return m_imuType == ImuType::ICM20602 ? readAndInjectImuIcm20602() : readAndInjectImuIcm42605();
}

/* =========================
 * MS5607 barometer (register map/compensation math from MS5607Sensor.cpp)
 * ========================= */

#define MS5607_ADDR       0x77
#define MS5607_RESET_CMD  0x1E
#define MS5607_PROM_READ  0xA0
#define MS5607_ADC_READ   0x00

// Not OSR 4096 (~9.04 ms): the temperature cycle then doesn't fit in the 10 ms tick
static constexpr uint8_t PRESSURE_CONV_CMD = 0x46;     // D1 conversion at OSR 2048
static constexpr uint32_t PRESSURE_CONV_DELAY_US = 4540;
static constexpr uint8_t TEMP_CONV_CMD = 0x50;         // D2 conversion at OSR 256
static constexpr uint32_t TEMP_CONV_DELAY_US = 600;

static bool baroReadUInt16(uint8_t promAddr, uint16_t& value) {
    Wire.beginTransmission(MS5607_ADDR);
    Wire.write(promAddr);
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom(MS5607_ADDR, (uint8_t)2) != 2) return false;
    const uint8_t hi = Wire.read();
    const uint8_t lo = Wire.read();
    value = ((uint16_t)hi << 8) | lo;
    return true;
}

bool Ms5607Mmc5603SensorPackage::readBarometerCalibration() {
    Wire.beginTransmission(MS5607_ADDR);
    Wire.write(MS5607_RESET_CMD);
    if (Wire.endTransmission() != 0) return false;
    delay(3); // datasheet: wait for internal register reload after reset

    const bool read = baroReadUInt16(MS5607_PROM_READ + 2, m_baroC1) &&
                      baroReadUInt16(MS5607_PROM_READ + 4, m_baroC2) &&
                      baroReadUInt16(MS5607_PROM_READ + 6, m_baroC3) &&
                      baroReadUInt16(MS5607_PROM_READ + 8, m_baroC4) &&
                      baroReadUInt16(MS5607_PROM_READ + 10, m_baroC5) &&
                      baroReadUInt16(MS5607_PROM_READ + 12, m_baroC6);
    const bool plausible = (m_baroC1 | m_baroC2 | m_baroC3 | m_baroC4 | m_baroC5 | m_baroC6) != 0 &&
                           (m_baroC1 & m_baroC2 & m_baroC3 & m_baroC4 & m_baroC5 & m_baroC6) != 0xFFFF;
    m_baroCalibrated = read && plausible;
    return m_baroCalibrated;
}

void Ms5607Mmc5603SensorPackage::setupBarometer() {
    if (readBarometerCalibration()) {
        m_debugStream->message("MS5607 initialized");
    } else {
        m_debugStream->error("MS5607 initialization failed");
    }
}

bool Ms5607Mmc5603SensorPackage::triggerPressureConversion() {
    Wire.beginTransmission(MS5607_ADDR);
    Wire.write(PRESSURE_CONV_CMD);
    const bool ok = Wire.endTransmission() == 0;
    m_baroConversionStartUs = micros();
    return ok;
}

bool Ms5607Mmc5603SensorPackage::triggerTemperatureConversion() {
    Wire.beginTransmission(MS5607_ADDR);
    Wire.write(TEMP_CONV_CMD);
    const bool ok = Wire.endTransmission() == 0;
    m_baroConversionStartUs = micros();
    return ok;
}

void Ms5607Mmc5603SensorPackage::waitForTemperatureConversion() const {
    while (micros() - m_baroConversionStartUs < TEMP_CONV_DELAY_US) {}
}

void Ms5607Mmc5603SensorPackage::waitForPressureConversion() const {
    while (micros() - m_baroConversionStartUs < PRESSURE_CONV_DELAY_US) {}
}

bool Ms5607Mmc5603SensorPackage::readBarometerAdc(uint32_t& value) const {
    Wire.beginTransmission(MS5607_ADDR);
    Wire.write(MS5607_ADC_READ);
    if (Wire.endTransmission(false) != 0) return false;
    // No stop: the next MS5607 command in read() follows with a repeated start
    if (Wire.requestFrom(MS5607_ADDR, (uint8_t)3, false) != 3) return false;
    const uint8_t b0 = Wire.read();
    const uint8_t b1 = Wire.read();
    const uint8_t b2 = Wire.read();
    value = ((uint32_t)b0 << 16) | ((uint32_t)b1 << 8) | b2;
    return true;
}

void Ms5607Mmc5603SensorPackage::computeAndInjectBarometerReading(const uint32_t rawPressure, const uint32_t rawTemperature) {
    // MS5607 datasheet integer algorithm; int64_t since the intermediates overflow 32 bits
    const int64_t dT = (int64_t)rawTemperature - ((int64_t)m_baroC5 << 8);
    const int64_t TEMP = 2000 + ((dT * (int64_t)m_baroC6) >> 23);
    const int64_t OFF  = ((int64_t)m_baroC2 << 17) + (((int64_t)m_baroC4 * dT) >> 6);
    const int64_t SENS = ((int64_t)m_baroC1 << 16) + (((int64_t)m_baroC3 * dT) >> 7);
    // P in 0.01 mbar
    const int64_t P = ((((int64_t)rawPressure * SENS) >> 21) - OFF) >> 15;

    const float temperatureC = (float)TEMP / 100.0f;
    const float pressureMbar = (float)P / 100.0f;

    m_barometer.inject((float)(temperatureC + Units::C_TO_K), 0.0f, (float)(pressureMbar * Units::MBAR_TO_PA));
}

/* =========================
 * MMC5603NJ magnetometer (MEMSIC datasheet Rev.B), free-running Continuous Measurement Mode (CMM) at 200 Hz
 * ========================= */

#define MMC5603_ADDR          0x30
#define MMC5603_XOUT0         0x00 // burst-read 6 bytes from here: Xout0,Xout1,Yout0,Yout1,Zout0,Zout1
#define MMC5603_STATUS1       0x18
#define MMC5603_ODR           0x1A
#define MMC5603_INT_CTRL_0    0x1B
#define MMC5603_INT_CTRL_1    0x1C
#define MMC5603_INT_CTRL_2    0x1D
#define MMC5603_PRODUCT_ID    0x39

#define MMC5603_STATUS1_MEAS_M_DONE  0x40
#define MMC5603_INT_CTRL_0_CMM_START 0xA0 // Cmm_freq_en (bit7) | Auto_SR_en (bit5) - calculates the ODR period counter; Auto_SR_en also gates Prd_set below
#define MMC5603_INT_CTRL_1_SW_RESET  0x80
#define MMC5603_INT_CTRL_1_BW01      0x01 // 3.5ms/measurement - BW00 (6.6ms) tops out ~150Hz even with periodic set, not enough for 200Hz
#define MMC5603_INT_CTRL_2_CMM_EN    0x1B // Cmm_en (bit4) | En_prd_set (bit3) | Prd_set[2:0]=011b (one SET/RESET per 100 samples)
#define MMC5603_PRODUCT_ID_VALUE     0x10

// ODR[7:0] takes the target Hz directly (datasheet "EXAMPLE OF CONTINUOUS MODE") - must be non-zero to enter CMM.
static constexpr uint8_t MMC5603_ODR_200HZ = 200;
static constexpr uint32_t MAG_FIRST_MEASUREMENT_TIMEOUT_US = 20000;

void Ms5607Mmc5603SensorPackage::setupMagnetometer() {
    writeRegister(MMC5603_ADDR, MMC5603_INT_CTRL_1, MMC5603_INT_CTRL_1_SW_RESET);
    delay(20); // datasheet: power-on/reset time is 20ms
    writeRegister(MMC5603_ADDR, MMC5603_INT_CTRL_1, MMC5603_INT_CTRL_1_BW01);

    const uint8_t productId = readRegister(MMC5603_ADDR, MMC5603_PRODUCT_ID);
    if (productId == MMC5603_PRODUCT_ID_VALUE) {
        m_debugStream->message("MMC5603NJ initialized");
    } else {
        m_debugStream->error("MMC5603NJ init failed, product ID = %d", productId);
    }

    // Order per datasheet "EXAMPLE OF CONTINUOUS MODE": ODR, then Cmm_freq_en, then Cmm_en
    writeRegister(MMC5603_ADDR, MMC5603_ODR, MMC5603_ODR_200HZ);
    writeRegister(MMC5603_ADDR, MMC5603_INT_CTRL_0, MMC5603_INT_CTRL_0_CMM_START);
    writeRegister(MMC5603_ADDR, MMC5603_INT_CTRL_2, MMC5603_INT_CTRL_2_CMM_EN);

    // Wait for the first measurement so the first read() doesn't inject a zero sample
    const uint32_t magSetupStartUs = micros();
    while ((readRegister(MMC5603_ADDR, MMC5603_STATUS1) & MMC5603_STATUS1_MEAS_M_DONE) == 0) {
        if (micros() - magSetupStartUs >= MAG_FIRST_MEASUREMENT_TIMEOUT_US) break;
    }
}

bool Ms5607Mmc5603SensorPackage::readAndInjectMag() {
    Wire.beginTransmission(MMC5603_ADDR);
    Wire.write(MMC5603_XOUT0);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(MMC5603_ADDR, (uint8_t)6) != 6) return false; // reading these also clears Meas_m_done

    uint8_t buf[6];
    for (unsigned char & i : buf) i = Wire.read();

    const uint16_t xRaw = ((uint16_t)buf[0] << 8) | buf[1];
    const uint16_t yRaw = ((uint16_t)buf[2] << 8) | buf[3];
    const uint16_t zRaw = ((uint16_t)buf[4] << 8) | buf[5];

    // 16-bit mode: unsigned, null field (0 Gauss) at 32768 counts, 1024 counts/Gauss.
    constexpr float NULL_FIELD_COUNTS = 32768.0f;
    constexpr float COUNTS_PER_GAUSS = 1024.0f;
    constexpr float GAUSS_TO_TESLA = 1e-4f;

    const Vector3D_s magneticFieldTesla = {
        ((float)xRaw - NULL_FIELD_COUNTS) / COUNTS_PER_GAUSS * GAUSS_TO_TESLA,
        ((float)yRaw - NULL_FIELD_COUNTS) / COUNTS_PER_GAUSS * GAUSS_TO_TESLA,
        ((float)zRaw - NULL_FIELD_COUNTS) / COUNTS_PER_GAUSS * GAUSS_TO_TESLA,
    };

    // Die temperature isn't part of CMM, so it isn't read
    m_magnetometer.inject(magneticFieldTesla, 0.0);
    return true;
}

/* =========================
 * I2C bus recovery
 * ========================= */

void Ms5607Mmc5603SensorPackage::recoverBus() {
    Wire.end();
    const I2cBusRecoveryResult result = recoverI2cBus(PIN_WIRE_SDA, PIN_WIRE_SCL);
    Wire.begin();
    Wire.setClock(400000);
    m_busRecoveries++;
    if (!m_busFaulted) {
        m_busFaulted = true;
        m_debugStream->warn("I2C transfer failed - bus cleared: SDA %s, SCL %s, %d clocks, %s", result.sdaWasLow ? "low" : "high", result.sclWasLow ? "low" : "high", result.clocks, result.released ? "released" : "still held");
    }
    if (result.released && !m_baroCalibrated && m_busRecoveries % 100 == 1 && readBarometerCalibration()) {
        m_debugStream->message("MS5607 initialized (after I2C recovery)");
    }
}

/* =========================
 * Ms5607Mmc5603SensorPackage
 * ========================= */

Ms5607Mmc5603SensorPackage::Ms5607Mmc5603SensorPackage(const Vector3DTransform* transform, const ImuType imuType, const bool hasMagnetometer, const Vector3DTransform* magTransform) :
    m_accelerometer(transform), m_gyroscope(transform), m_magnetometer(magTransform ? magTransform : transform),
    m_imuType(imuType), m_hasMagnetometer(hasMagnetometer) {}

void Ms5607Mmc5603SensorPackage::setup(DebugStream* debugStream, WatchdogTimer* watchdog) {
    m_debugStream = debugStream;

    const I2cBusRecoveryResult busClear = recoverI2cBus(PIN_WIRE_SDA, PIN_WIRE_SCL);
    if (busClear.sdaWasLow || !busClear.released) {
        m_debugStream->warn("I2C bus was held at boot: SDA %s, SCL %s, %d clocks, %s", busClear.sdaWasLow ? "low" : "high", busClear.sclWasLow ? "low" : "high", busClear.clocks, busClear.released ? "released" : "still held");
    }

    Wire.begin();
    Wire.setClock(400000);

    setupImu();
    setupBarometer();
    if (m_hasMagnetometer) setupMagnetometer();

    triggerPressureConversion();
}

void Ms5607Mmc5603SensorPackage::read() {
    waitForPressureConversion();

    // Mag + IMU reads overlap the ~600 us temperature conversion
    uint32_t rawPressure = 0, rawTemperature = 0;
    bool baroOk = readBarometerAdc(rawPressure);
    baroOk = triggerTemperatureConversion() && baroOk;
    bool othersOk = true;
    if (m_hasMagnetometer) othersOk = readAndInjectMag();
    othersOk = readAndInjectImu() && othersOk;
    waitForTemperatureConversion();
    baroOk = readBarometerAdc(rawTemperature) && baroOk;
    if (baroOk && m_baroCalibrated) {
        computeAndInjectBarometerReading(rawPressure, rawTemperature);
    } else {
        m_barometer.invalidate();
    }

    if (!baroOk || !othersOk || !m_baroCalibrated) {
        recoverBus();
    } else if (m_busFaulted) {
        m_busFaulted = false;
        m_debugStream->message("I2C reads working again (%lu bus clears)", (unsigned long)m_busRecoveries);
        m_busRecoveries = 0;
    }
    triggerPressureConversion();
}

Accelerometer* Ms5607Mmc5603SensorPackage::getAccelerometer() {
    return &m_accelerometer;
}

Gyroscope* Ms5607Mmc5603SensorPackage::getGyroscope() {
    return &m_gyroscope;
}

Barometer* Ms5607Mmc5603SensorPackage::getBarometer() {
    return &m_barometer;
}

Magnetometer* Ms5607Mmc5603SensorPackage::getMagnetometer() {
    return m_hasMagnetometer ? &m_magnetometer : nullptr;
}
