#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOSYSTEMCLOCK_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOSYSTEMCLOCK_H

#include <Arduino.h>
#include "Avionics.h"
#include "SystemClock.h"

class ArduinoSystemClock : public SystemClock {
public:
    uint_avionics_time_t currentRuntimeMs() override {
        return millis();
    }
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOSYSTEMCLOCK_H
