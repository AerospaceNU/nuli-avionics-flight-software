#ifndef ARDUINOAVIONICSHELPER_H
#define ARDUINOAVIONICSHELPER_H

#include "Arduino.h"
#include <initializer_list>

static void disableChipSelectPins(const std::initializer_list<uint32_t> pins) {
    for (const auto pin : pins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
    }
}

#endif //ARDUINOAVIONICSHELPER_H
