

#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H

#include "Avionics.h"
#include "ConstantsUnits.h"
#include "DebugStream.h"
#include <iostream>

class DesktopDebug : public DebugStream {
public:

    void setup() override {

    }

    void print(const char* str) override {
        std::cout << (str);
    }

    void print(int64_t num) override {
        std::cout << (num);
    }

    void print(double num) override {
        std::cout << (num);
    }
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
