#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSMODULATION_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSMODULATION_H

#include <Avionics.h>

class AprsModulation {
public:
    explicit AprsModulation(uint8_t transmitPin, const char *callsign);

    void setup() const;

    void transmit(const char* str);

private:
    uint8_t m_transmitPin;
    char m_callsign[10];
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSMODULATION_H
