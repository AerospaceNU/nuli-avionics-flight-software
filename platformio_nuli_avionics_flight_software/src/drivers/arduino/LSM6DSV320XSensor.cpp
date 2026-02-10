#include <Arduino.h>
#include "LSM6DSV320XSensor.h"
#include <SPI.h>

// Basic device register addresses
#define WHO_AM_I_REG       0x0F  // Contains device ID
#define CTRL1_XL_REG       0x10  // Low G Accelerometer control
#define CTRL2_G_REG        0x11  // Gyroscope control
#define CTRL1_XL_HG        0x4E  // High G Accelerometer control
#define CTRL6              0x15  // set dps & LPF of Gyro
#define CTRL8              0x17  // set Accel low G +- selection 0x03
#define OUTX_L_A           0x28  // Low G Accel X low byte (multi-byte)
#define UI_OUTX_L_A_OIS_HG 0x34  // High G Accel X low byte (muli-byte)
#define OUTX_L_G           0x22  // Gyro X low byte (multi-byte)


LSM6DSV320XSensor::LSM6DSV320XSensor(const Vector3DTransform* transform, const uint8_t chipSelectPin, SPIClass *spi) :
    m_highGAccelerometer(transform),
    m_lowGAccelerometer(transform),
    m_gyroscope(transform),
    m_chipSelectPin(chipSelectPin),
    m_spi(spi) {}

void LSM6DSV320XSensor::setup(DebugStream* debugStream) {
    pinMode(m_chipSelectPin, OUTPUT);
    digitalWrite(m_chipSelectPin, HIGH);
    m_spi->begin();

    //setting up registers

    // probably don't need these first three as they are defaults
    writeRegister(CTRL1_XL_REG, 0x00); // Low G Accel power down
    writeRegister(CTRL1_XL_HG, 0x00);  // High G Accel power down
    writeRegister(CTRL2_G_REG, 0x00);  // Gyro Power down

    // Configure Sensors

    // +- 16G Low-G Accel
    writeRegister(CTRL8, 0x03);
    // Gyro +-4000 dps
    writeRegister(CTRL6, 0x0D);

    // Low G High performance 120Hz Accel 16 G
    writeRegister(CTRL1_XL_REG, 0x06);
    // High G High performance 480Hz Accel 320 G
    writeRegister(CTRL1_XL_HG, 0x9C);
    // High performance 120Hz Gyro 4000 dps
    writeRegister(CTRL2_G_REG, 0x06);
    //check for correct read
    uint8_t whoAmI = readRegister(WHO_AM_I_REG);
    if (whoAmI == 0x73) {
        debugStream->message("LSM6DSV320X initialized");
    }
    else {
        debugStream->message("LSM6DSV320X initialization failed, unexpected device ID: %d", whoAmI);
    }
}


void LSM6DSV320XSensor::read() {
    //need to get data from sensor

    // need to convert data to correct units

    //need to inject data into injector class member variables

}

Accelerometer* LSM6DSV320XSensor::getHighGAccelerometer() {
    return &m_highGAccelerometer;
}

Accelerometer* LSM6DSV320XSensor::getLowGAccelerometer() {
    return &m_lowGAccelerometer;
}

Gyroscope* LSM6DSV320XSensor::getGyroscope() {
    return &m_gyroscope;
}

void LSM6DSV320XSensor::writeRegister(uint8_t reg, uint8_t value) {
    digitalWrite(m_chipSelectPin, LOW);
    SPI.transfer(reg & 0x7F);  // Clear MSB for write
    SPI.transfer(value);
    digitalWrite(m_chipSelectPin, HIGH);
}

uint8_t LSM6DSV320XSensor::readRegister(uint8_t reg) {
    digitalWrite(m_chipSelectPin, LOW);
    SPI.transfer(0x80 | reg);  // Set MSB for read
    uint8_t val = SPI.transfer(0x00);
    digitalWrite(m_chipSelectPin, HIGH);
    return val;
}

// Read multiple bytes
void LSM6DSV320XSensor::readBytes(uint8_t reg, uint8_t *buffer, uint8_t length) {
    digitalWrite(m_chipSelectPin, LOW);
    SPI.transfer(0x80 | reg);  // Read (auto increment set up by default)
    for (uint8_t i = 0; i < length; i++) {
        buffer[i] = SPI.transfer(0x00);
    }
    digitalWrite(m_chipSelectPin, HIGH);
}





