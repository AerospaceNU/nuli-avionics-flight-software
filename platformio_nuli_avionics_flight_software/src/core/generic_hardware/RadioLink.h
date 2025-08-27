#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RADIOLINK_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RADIOLINK_H

#include <Avionics.h>
#include "GenericSensor.h"

class RadioLink {
public:
    virtual void setup() {}

    virtual void loopOnce() {}

    virtual bool hasNewData() { return false; }

    virtual uint32_t getData(uint8_t* data, uint32_t maxLength) { return 0; };

    virtual void transmit(uint8_t* data, uint32_t length) {};

    virtual void setfrequency(float frequency) {};

    virtual void setBandwidth(float bandwidth) {};

    /**
     * @brief This is LoRa specific. It will do nothing if called with a non-LoRa module
     * @param spreadingFactor A number 5-12 inclusive. # of symbols
     */
    virtual void setSpreadingFactor(float spreadingFactor) {};
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RADIOLINK_H
