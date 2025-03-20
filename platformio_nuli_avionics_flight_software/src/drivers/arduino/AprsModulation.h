#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSMODULATION_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSMODULATION_H

#include <Avionics.h>
#include "../../core/generic_hardware/RadioLink.h"

class AprsModulation : public RadioLink {
public:
    explicit AprsModulation(uint8_t transmitPin, const char* callsign);

    void setup() override;

    void transmit(uint8_t* data, uint32_t length) override;

    const char* getCallsign();

private:
    uint8_t m_transmitPin;
    char m_callsign[10];
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSMODULATION_H
