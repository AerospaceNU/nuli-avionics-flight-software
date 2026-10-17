#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RFM9XRADIO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RFM9XRADIO_H

#include "core/generic_hardware/GenericHardware.h"
#include "RadioLib.h"

#define RADIO_BUFFER_SIZE 1024

class RFM9xRadio : public RadioLink {
public:
    RFM9xRadio(float frequency);

    void setup() override;

    /** @brief Buffers any newly received packet */
    void loopOnce() override;

    bool hasNewData() override;

    uint32_t getData(uint8_t *data, uint32_t maxLength) override;

    void transmit(uint8_t *data, uint32_t length) override;

protected:
private:
    uint16_t m_packetNumber = 0;
    uint32_t m_dataLength = 0;
    uint8_t m_inBuffer[RADIO_BUFFER_SIZE] = {0};

    RFM95 m_radio = nullptr;

    float m_frequency = 915;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RFM9XRADIO_H
