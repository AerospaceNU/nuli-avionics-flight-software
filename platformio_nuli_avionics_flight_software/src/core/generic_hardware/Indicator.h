#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATOR_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATOR_H

#include "Avionics.h"

class Indicator {
public:
    enum IndicatorType {
        NONE,
        AUDIO,
        VISUAL,
    };

    virtual void setup() {}

    virtual void on() = 0;

    virtual void off() = 0;

    IndicatorType getType() {
        return type;
    }

protected:
    IndicatorType type = NONE;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATOR_H
