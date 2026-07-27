#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RADIOLINK_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RADIOLINK_H

#include <Avionics.h>
#include "GenericAvionicsHardware.h"

class RadioLink : public GenericAvionicsHardware {
public:
    enum class RadioLinkStatus {
        IDLE,
        RX_LISTENING, ///< Waiting for incoming message
        RX_ACTIVE, ///< Start of message heard, don't stop now
        TX_ACTIVE, ///< Sending message
    };

    struct RadioMessage {
        uint8_t data[255];
        uint8_t length; // actual received length, 0-255
        int16_t rssi;
        float snr;
    };

    virtual bool startTransmit(void* data, uint32_t length) { return false; };

    virtual bool startReceive() { return false; };

    virtual bool setFrequency(float frequencyMHz) { return false; };

    virtual bool setSpreadingFactor(uint8_t spreadingFactor) { return false; };

    virtual bool isMessageAvailable() const { return false; }

    virtual RadioMessage readMessage() const { return {{}, 0, 0, 0}; };  // Clears the isMessageAvailable flag

    virtual RadioLinkStatus status() const { return RadioLinkStatus::IDLE; }
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RADIOLINK_H
