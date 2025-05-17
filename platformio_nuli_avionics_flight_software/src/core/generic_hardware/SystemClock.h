#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SYSTEMCLOCK_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SYSTEMCLOCK_H

#include "Avionics.h"
#include "ConstantsUnits.h"

class SystemClock {
public:
    virtual uint32_t currentRuntimeMs() = 0;

};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SYSTEMCLOCK_H
