#include "AprsModulation.h"
//#include <Arduino.h>
#include "iostream"
#include <thread>
#include <chrono>


AprsModulation::AprsModulation(uint8_t transmitPin, const char *callsign) {
    uint32_t i = 0;
    for(; i < sizeof(m_callsign) - 1 && callsign[i] != '\0'; i++) {
        m_callsign[i] = callsign[i];
    }
    m_callsign[i] = '\0';

}

void AprsModulation::setup() const {

}

void AprsModulation::transmit(const char* str) {
    std::cout << str << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

const char* AprsModulation::getCallsign() {
    return m_callsign;
}
