#include "ArduinoFram.h"

ArduinoFram::ArduinoFram(int8_t csPin) : m_AdafruitFram(csPin) {

}

void ArduinoFram::setup(DebugStream *debugStream) {
    if (m_AdafruitFram.begin()) {
        debugStream->message("FRAM initialized");
    } else {
        debugStream->error("FRAM initialization failed");
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


