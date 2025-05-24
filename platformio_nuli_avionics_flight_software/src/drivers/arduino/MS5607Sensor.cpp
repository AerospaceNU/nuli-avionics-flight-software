#include "MS5607Sensor.h"
#include "ConstantsUnits.h"
#include <Arduino.h>

// Much of this code was taken from here: https://github.com/UravuLabs/MS5607/blob/master/LICENSE

#define R_ADC  0X00         // adc read command
#define PROM_READ  0xA0     // prom read command
#define RESET 0x1E          // soft reset command

void MS5607Sensor::setup() {
    Serial.println("Starting barometric sensor");
    Wire.begin();
    Wire.setClock(400000);
    if (readCalibration()) {
        Serial.println("Started");
    } else {
        Serial.println("Failed");
    }
    setOSR(256);
}


void MS5607Sensor::read() {
    readDigitalValue();
    m_pressurePa = getPressure() * Units::MBAR_TO_PA;
    m_temperatureK = getTemperature() + Units::C_TO_K;
    calculateAltitude();
}

// read raw digital values of temp & pressure from MS5607

bool MS5607Sensor::readDigitalValue() {
    // @todo make this start the pressure reading at the highest OSR
    if (startConversion(CONV_D1)) {
        if (startMeasurement()) {
            getDigitalValue(DP);
        }
    } else {
        return false;
    }

    if (startConversion(CONV_D2)) {
        if (startMeasurement()) {
            getDigitalValue(DT);
        }
    } else {
        return false;
    }
    return true;
}

// send command to start measurement
bool MS5607Sensor::startMeasurement() const {
    Wire.beginTransmission(MS5607_ADDR);
    Wire.write(R_ADC);
    char error = Wire.endTransmission();
    if (error == 0) {
        return true;
    } else {
        Serial.println("dsf");
        return false;
    }
}

// send command to start conversion of temp/pressure
bool MS5607Sensor::startConversion(char CMD) const {
    Wire.beginTransmission(MS5607_ADDR);
    Wire.write(CMD);
    char error = Wire.endTransmission();
    if (error == 0) {
        delayMicroseconds(Conv_Delay);
        return true;
    } else {
        return false;
    }
}


bool MS5607Sensor::resetDevice() const {
    Wire.beginTransmission(MS5607_ADDR);
    Wire.write(RESET);
    char error = Wire.endTransmission();
    if (error == 0) {
        delay(3);     // wait for internal register reload
        return true;
    } else {
        return false;
    }
}

// read calibration data from PROM
bool MS5607Sensor::readCalibration() {
    if (resetDevice() &&
        readUInt_16(PROM_READ + 2, C1) &&
        readUInt_16(PROM_READ + 4, C2) &&
        readUInt_16(PROM_READ + 6, C3) &&
        readUInt_16(PROM_READ + 8, C4) &&
        readUInt_16(PROM_READ + 10, C5) &&
        readUInt_16(PROM_READ + 12, C6)
            ) {
        return true;
    } else {
        return false;
    }
}

// convert raw data into unsigned int
bool MS5607Sensor::readUInt_16(char address, unsigned int &value) {
    unsigned char data[2];    //4bit
    data[0] = address;
    if (readBytes(data, 2)) {
        value = (((unsigned int) data[0] * (1 << 8)) | (unsigned int) data[1]);
        return true;
    }
    value = 0;
    return false;
}

// read number of bytes over i2c
bool MS5607Sensor::readBytes(unsigned char* values, uint8_t length) const {
    uint8_t x;

    Wire.beginTransmission(MS5607_ADDR);
    Wire.write(values[0]);

    uint8_t error = Wire.endTransmission();
    if (error == 0) {
        Wire.requestFrom(MS5607_ADDR, length);
//        while (!Wire.available()); // wait until bytes are ready
        waitForI2C();
        for (x = 0; x < length; x++) {
            values[x] = Wire.read();
        }
        return true;
    }
    return false;
}


bool MS5607Sensor::getDigitalValue(unsigned long &value) const {
    uint8_t x, length = 3;
    unsigned char data[3];
    Wire.requestFrom(MS5607_ADDR, length);
    waitForI2C();
    for (x = 0; x < length; x++) {
        data[x] = Wire.read();
    }
    value = (unsigned long) data[0] * 1 << 16 | (unsigned long) data[1] * 1 << 8 | (unsigned long) data[2];
    return true;
}

float MS5607Sensor::getTemperature() {
    dT = (float) DT - ((float) C5) * ((int) 1 << 8);
    TEMP = 2000.0 + dT * ((float) C6) / (float) ((long) 1 << 23);
    return TEMP / 100;
}

float MS5607Sensor::getPressure() {
    dT = (float) DT - ((float) C5) * ((int) 1 << 8);
    TEMP = 2000.0 + dT * ((float) C6) / (float) ((long) 1 << 23);
    OFF = (((int64_t) C2) * ((long) 1 << 17)) + dT * ((float) C4) / ((int) 1 << 6);
    SENS = ((float) C1) * ((long) 1 << 16) + dT * ((float) C3) / ((int) 1 << 7);
    float pa = (float) ((float) DP / ((long) 1 << 15));
    float pb = (float) (SENS / ((float) ((long) 1 << 21)));
    float pc = pa * pb;
    float pd = (float) (OFF / ((float) ((long) 1 << 15)));
    P = pc - pd;
    return P / 100;
}

// set OSR and select corresponding values for conversion commands & delay
void MS5607Sensor::setOSR(uint16_t OSR_U) {
    this->OSR = OSR_U;
    switch (OSR) {
        case 256:
            CONV_D1 = 0x40;
            CONV_D2 = 0x50;
            Conv_Delay = 600;
            break;
        case 512:
            CONV_D1 = 0x42;
            CONV_D2 = 0x52;
            Conv_Delay = 1170;
            break;
        case 1024:
            CONV_D1 = 0x44;
            CONV_D2 = 0x54;
            Conv_Delay = 2280;
            break;
        case 2048:
            CONV_D1 = 0x46;
            CONV_D2 = 0x56;
            Conv_Delay = 4540;
            break;
        case 4096:
            CONV_D1 = 0x48;
            CONV_D2 = 0x58;
            Conv_Delay = 9040;
            break;
        default:
            CONV_D1 = 0x40;
            CONV_D2 = 0x50;
            Conv_Delay = 600;
            break;
    }
}

void MS5607Sensor::waitForI2C() const {
    uint32_t end = millis() + 30;
    while (!Wire.available() && millis() < end);
}


