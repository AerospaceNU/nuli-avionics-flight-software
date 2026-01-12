#include "LSM6DSV320X.h"

LSM6DSV320X::LSM6DSV320X(uint8_t csPin) : _csPin(csPin) {}


void LSM6DSV320X::begin() {
  pinMode(_csPin, OUTPUT);
  digitalWrite(_csPin, HIGH);
  SPI.begin();
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE3));
}

uint8_t LSM6DSV320X::readRegister(uint8_t reg) {
  digitalWrite(_csPin, LOW);
  SPI.transfer(0x80 | reg);  // Set MSB for read
  uint8_t val = SPI.transfer(0x00);
  digitalWrite(_csPin, HIGH);
  return val;
}

void LSM6DSV320X::writeRegister(uint8_t reg, uint8_t value) {
  digitalWrite(_csPin, LOW);
  SPI.transfer(reg & 0x7F);  // Clear MSB for write
  SPI.transfer(value);
  digitalWrite(_csPin, HIGH);
}

// Read multiple bytes
void LSM6DSV320X::readBytes(uint8_t reg, uint8_t *buffer, uint8_t length) {
  digitalWrite(_csPin, LOW);
  SPI.transfer(0x80 | reg);  // Read (auto increment set up by default)
  for (uint8_t i = 0; i < length; i++) {
    buffer[i] = SPI.transfer(0x00);
  }
  digitalWrite(_csPin, HIGH);
}

bool LSM6DSV320X::testConnection() {
  uint8_t id = readRegister(WHO_AM_I_REG);
  return (id == 0x73);
}

void LSM6DSV320X::configureSensor() {
  // probably dont need these first two as they are defaults
  writeRegister(CTRL1_XL_REG, 0x00); // Accel power down
  writeRegister(CTRL2_G_REG, 0x00); // Gyro Power down

  // Configure Sensors

  // +- 16G Low-G Accel
  writeRegister(CTRL8, 0x03); 
  // Gyro +-4000 dps
  writeRegister(CTRL6, 0x0D);

  // Low G High performance 120Hz Accel
  writeRegister(CTRL1_XL_REG, 0x06);
  // High G High performance 480Hz Accel
  writeRegister(CTRL1_XL_HG, 0x9C);
  // High performance 120Hz Gyro
  writeRegister(CTRL2_G_REG, 0x06);

}

// Read 3-axis Low G accelerometer
void LSM6DSV320X::readLowGAccel(int16_t &ax, int16_t &ay, int16_t &az) {
  uint8_t buf[6];
  readBytes(OUTX_L_A, buf, 6);
  ax = (int16_t)(buf[1] << 8 | buf[0]);
  ay = (int16_t)(buf[3] << 8 | buf[2]);
  az = (int16_t)(buf[5] << 8 | buf[4]);
}

// Read 3-axis High G accelerometer
void LSM6DSV320X::readHighGAccel(int16_t &ax, int16_t &ay, int16_t &az) {
  uint8_t buf[6];
  readBytes(UI_OUTX_L_A_OIS_HG, buf, 6);
  ax = (int16_t)(buf[1] << 8 | buf[0]);
  ay = (int16_t)(buf[3] << 8 | buf[2]);
  az = (int16_t)(buf[5] << 8 | buf[4]);
}

// Read 3-axis gyroscope
void LSM6DSV320X::readGyro(int16_t &gx, int16_t &gy, int16_t &gz) {
  uint8_t buf[6];
  readBytes(OUTX_L_G, buf, 6);
  gx = (int16_t)(buf[1] << 8 | buf[0]);
  gy = (int16_t)(buf[3] << 8 | buf[2]);
  gz = (int16_t)(buf[5] << 8 | buf[4]);
}