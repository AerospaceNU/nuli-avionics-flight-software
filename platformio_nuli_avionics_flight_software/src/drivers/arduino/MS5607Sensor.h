#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS5607Sensor_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS5607Sensor_H

#include "../../core/generic_hardware/Barometer.h"
#include <Wire.h>

/**
 * @class MS5607Sensor
 * @brief An implementation of Barometer for the MS8607 barometer.
 */
class MS5607Sensor : public Barometer {
public:
    /**
     * @brief Initialize the sensor
     * @details Enabling any peripherals, confirm sensor is talking, set configuration registers on the sensor
     */
    void setup() override;

    /**
     * @brief Read data from the sensor
     * @details Read in one reading from the sensor, and convert the data to usefully units/numbers.
     * Currently is allowed to block the loop to wait for data from the sensor for a few ms.
     */
    void read() override;

    /**
     * @brief Sets the over sampling rate (currently hardcoded)
     * @param OSR_U (2048, 4096, etc)
     */
    void setOSR(uint16_t OSR_U);

private:
    uint16_t MS5607_ADDR = 0X77;      // default device address of MS5607 (CBS == HIGH)
    uint16_t OSR = 4096;              // default over sampling ratio
    uint16_t CONV_D1 = 0x48;          // corresponding temp conv. command for OSR
    uint16_t CONV_D2 = 0x58;          // corresponding pressure conv. command for OSR
    uint16_t Conv_Delay = 9040;          // corresponding conv. delay for OSR

    unsigned int C1, C2, C3, C4, C5, C6;
    unsigned long DP, DT;
    float dT, TEMP, P;
    int64_t OFF, SENS;


    bool readDigitalValue();
    float getTemperature();
    float getPressure();
    bool resetDevice() const;
    bool readCalibration();
    bool readUInt_16(char address, unsigned int &value);
    bool readBytes(unsigned char* values, uint8_t length) const;
    bool startConversion(char CMD) const;
    bool startMeasurement() const;
    bool getDigitalValue(unsigned long &value) const;
    void waitForI2C() const;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS5607Sensor_H
