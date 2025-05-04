#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RFM9XRADIO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RFM9XRADIO_H

#include "../src/core/generic_hardware/RadioLink.h"
#include "RadioLib.h"

#define RADIO_BUFFER_SIZE 1024

class RFM9xRadio : public RadioLink {
public:
    RFM9xRadio(float frequency);

    /**
     * @brief Setup radio hardware and pins
     */
    void setup() override;

    /**
     * @brief Ask the radio if there is any new data, if so buffer it.
     */
    void loopOnce() override;

    /**
     * @brief Checks if there is data in the internal buffer for the user to
     * retrieve. Data comes from the radio.
     * @return True if data in buffer.
     */
    bool hasNewData() override;

    /**
     * @brief Retrieve from the buffer, and then give it to the caller.
     * @param data Where to put buffered data into.
     * @param maxLength Maximum length, in bytes, to return to user.
     * @return Amount of data retrieved.
     */
    uint32_t getData(uint8_t *data, uint32_t maxLength) override;

    /**
     * @brief Sends data over radio
     * @param data Data to send
     * @param length Amount of data to send
     */
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
