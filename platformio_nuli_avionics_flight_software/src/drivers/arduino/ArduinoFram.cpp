#include "ArduinoFram.h"

ArduinoFram::ArduinoFram(int8_t csPin) : m_AdafruitFram(csPin) {

}

void ArduinoFram::setup() {
    if (m_AdafruitFram.begin()) {
        Serial.println("Found SPI FRAM");
    } else {
        Serial.println("No SPI FRAM found ... check your connections\r\n");
    }
}

void ArduinoFram::write(uint32_t address, const uint8_t* buffer, uint32_t length) {
    m_AdafruitFram.writeEnable(true);
    m_AdafruitFram.write(address, buffer, length);
    m_AdafruitFram.writeEnable(false);
}

void ArduinoFram::read(uint32_t address, uint8_t* buffer, uint32_t length) {
    m_AdafruitFram.read(address, buffer, length);
}


