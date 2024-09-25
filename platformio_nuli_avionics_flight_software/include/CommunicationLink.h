#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_COMMUNICATIONLINK_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_COMMUNICATIONLINK_H

#include <Avionics.h>
#include <GenericSensor.h>

class CommunicationLink {
public:
    void setup() {}

    void read() {}
};

class RadioTransmitterLink : public CommunicationLink {
};

class SerialConnectionLink : public CommunicationLink {

};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_COMMUNICATIONLINK_H
