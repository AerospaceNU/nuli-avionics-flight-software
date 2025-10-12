#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SYSTEMCLOCK_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SYSTEMCLOCK_H

#include "Avionics.h"
#include "ConstantsUnits.h"

class SystemClock {
public:
    virtual ~SystemClock() = default;

    virtual void setup(DebugStream *debugStream) {

    }

    virtual uint32_t currentRuntimeMs() = 0;

    virtual uint32_t currentRuntimeUs() = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SYSTEMCLOCK_H
