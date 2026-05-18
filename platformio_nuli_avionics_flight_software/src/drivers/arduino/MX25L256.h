#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MX25L256_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MX25L256_H

#include "FlashMemoryCommon.h"

class MX25L256 final : public FlashMemoryCommon {
public:
    explicit MX25L256(uint8_t chipSelectPin);
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MX25L256_H
