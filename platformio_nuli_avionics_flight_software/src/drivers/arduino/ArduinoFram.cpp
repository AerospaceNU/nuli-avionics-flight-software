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

bool ArduinoFram::ready() {
    return true;
}

void ArduinoFram::write(uint32_t address, const uint8_t* buffer, uint32_t length) {
    m_AdafruitFram.write(address, buffer, length);
}

void ArduinoFram::read(uint32_t address, uint8_t* buffer, uint32_t length) {
    m_AdafruitFram.read(address, buffer, length);
}


