#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS5607Sensor_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS5607Sensor_H

#include "core/generic_hardware/GenericHardware.h"
#include <Wire.h>

/** @brief MS5607 barometer driver (I2C) */
class MS5607Sensor : public Barometer {
public:
    void setup(DebugStream* debugStream) override;

    void read() override;

    /** @param OSR_U 256, 512, 1024, 2048 or 4096; anything else selects 256 */
    void setOSR(uint16_t OSR_U);

private:
    uint16_t MS5607_ADDR = 0X77;      // CSB high
    uint16_t OSR = 4096;
    uint16_t CONV_D1 = 0x48;          // pressure
    uint16_t CONV_D2 = 0x58;          // temperature
    uint16_t Conv_Delay = 9040;          // us

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
