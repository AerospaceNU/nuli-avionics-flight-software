#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSMODULATION_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSMODULATION_H

#include <Avionics.h>


class AprsModulation {
public:
    explicit AprsModulation(uint8_t transmitPin);

    void setup() const;

    void transmit(const char *str, uint32_t length);

private:
    uint8_t m_transmitPin;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSMODULATION_H
