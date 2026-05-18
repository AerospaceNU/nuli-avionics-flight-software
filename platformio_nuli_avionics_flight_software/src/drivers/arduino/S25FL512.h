#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_S25FL512_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_S25FL512_H

#include "FlashMemoryCommon.h"

class S25FL512 final : public FlashMemoryCommon {
public:
    explicit S25FL512(uint8_t chipSelectPin);
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_S25FL512_H
