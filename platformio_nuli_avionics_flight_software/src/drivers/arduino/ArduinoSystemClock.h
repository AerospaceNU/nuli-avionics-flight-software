#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOSYSTEMCLOCK_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOSYSTEMCLOCK_H

#include <Arduino.h>
#include "Avionics.h"
#include "core/generic_hardware/SystemClock.h"

class ArduinoSystemClock final : public SystemClock {
public:
    uint32_t currentRuntimeMs() override {
        return millis();
    }
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOSYSTEMCLOCK_H
