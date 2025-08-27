#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SX126XRADIO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SX126XRADIO_H

#include "../src/core/generic_hardware/RadioLink.h"
#include "RadioLib.h"

#define RADIO_BUFFER_SIZE 1024

class SX126xRadio : public RadioLink {
public:

    SX126xRadio(
        uint8_t chipSelect,
        uint8_t dio1,
        uint8_t reset,
        uint8_t busy,
        uint8_t rxEnable,
        uint8_t txEnable);

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


    void setFrequency(float frequency) override;

    void setBandwidth(float bandwidth) override;

    void setSpreadingFactor(float spreadingFactor) override;

protected:
private:
    uint16_t m_packetNumber = 0;
    uint32_t m_dataLength = 0;
    uint8_t m_inBuffer[RADIO_BUFFER_SIZE] = {0};

    SX1262 m_radio = nullptr;

    float m_frequency = 915;
    float m_bandwidth;
    float m_spreadingFactor;
    //pins
    uint8_t m_chipSelect;
    uint8_t m_dio1;
    uint8_t m_reset;
    uint8_t m_busy;
    uint8_t m_rxEnable;
    uint8_t m_txEnable;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SX126XRADIO_H