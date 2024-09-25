#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FLASHMEMORY_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FLASHMEMORY_H

#include <Avionics.h>
#include <BaseSensor.h>

class FlashMemory {
public:
    virtual void read() {}

    virtual void setup() {}
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FLASHMEMORY_H
