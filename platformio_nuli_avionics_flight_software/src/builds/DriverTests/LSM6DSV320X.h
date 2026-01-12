#ifndef LSM6DSV320X_H
#define LSM6DSV320X_H

#include <Arduino.h>
#include <SPI.h>

// Chip select pin for SPI
#define LSM6DSV320X_CS_PIN 10

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

class LSM6DSV320X {
public:
  LSM6DSV320X(uint8_t csPin = LSM6DSV320X_CS_PIN);

  void begin();
  uint8_t readRegister(uint8_t reg);
  void writeRegister(uint8_t reg, uint8_t value);
  void readBytes(uint8_t reg, uint8_t *buffer, uint8_t length);

  bool testConnection();
  void configureSensor();

  void readLowGAccel(int16_t &ax, int16_t &ay, int16_t &az);
  void readHighGAccel(int16_t &ax, int16_t &ay, int16_t &az);
  void readGyro(int16_t &gx, int16_t &gy, int16_t &gz);

private:
  uint8_t _csPin;
};

#endif
