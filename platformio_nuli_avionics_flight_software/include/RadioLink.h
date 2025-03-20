#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RADIOLINK_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RADIOLINK_H

#include <Avionics.h>
#include <GenericSensor.h>

class RadioLink {
public:
    virtual void setup() {}

    virtual void loopOnce() {}

    virtual bool hasNewData() { return false; }

    virtual uint32_t getData(uint8_t* data, uint32_t maxLength) { return 0; };

    virtual void transmit(uint8_t* data, uint32_t length) {};
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RADIOLINK_H
