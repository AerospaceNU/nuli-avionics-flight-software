#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SX1262RADIO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SX1262RADIO_H

#include "core/generic_hardware/GenericHardware.h"
#include <RadioLib.h>

/**
 * @class SX1262Radio
 * @brief Radio driver for SX1262-based modules (SeriousGoose uses an EBYTE E22-900MM22S)
 * @details Uses RadioLib. DIO2/DIO3 aren't wired to the MCU, so RXEN/TXEN are driven directly via RadioLib's setRfSwitchPins() instead of the chip's internal RF-switch feature; the onboard TCXO is powered via begin()'s tcxoVoltage argument.
 */
class SX1262Radio : public RadioLink {
public:
    /**
     * @brief Creates an SX1262 radio driver
     * @param csPin SPI chip select (NSS)
     * @param dio1Pin DIO1 interrupt pin - fires on TX done / RX done
     * @param resetPin Chip reset pin (NRST)
     * @param busyPin BUSY status pin
     * @param rxEnPin RF switch receive-enable pin
     * @param txEnPin RF switch transmit-enable pin
     * @param frequencyMHz Center frequency in MHz
     */
    SX1262Radio(uint8_t csPin, uint8_t dio1Pin, uint8_t resetPin, uint8_t busyPin,
                uint8_t rxEnPin, uint8_t txEnPin, float frequencyMHz);

    /**
     * @brief Initialize the radio and start listening for incoming packets
     */
    void setup(DebugStream* debugStream, WatchdogTimer* watchdog) override;

    /**
     * @brief Service the last TX/RX operation flagged by the DIO1 interrupt
     * @details If a receive just completed, buffers the message and re-arms receive; if a transmit just completed, re-arms receive. Must be called regularly.
     */
    void run() override;

    /**
     * @brief Start sending data over the radio
     * @details Non-blocking - completion is picked up by run() via the DIO1 interrupt.
     * @param data Data to send
     * @param length Amount of data to send, in bytes
     */
    bool startTransmit(void* data, uint32_t length) override;

    /**
     * @brief Put the radio into continuous receive mode
     */
    bool startReceive() override;

    /**
     * @brief Change the radio's center frequency
     * @details Puts the radio into standby first (required to accept a new frequency), then resumes listening afterward if it was previously receiving.
     * @param frequencyMHz Center frequency in MHz
     */
    bool setFrequency(float frequencyMHz) override;

    /**
     * @brief Change the LoRa spreading factor
     * @details Puts the radio into standby first (required to accept new modulation parameters), then resumes listening afterward if it was previously receiving.
     * @param spreadingFactor LoRa spreading factor, 5-12
     */
    bool setSpreadingFactor(uint8_t spreadingFactor) override;

    bool isMessageAvailable() const override;

    /**
     * @brief Retrieve the last received message. Clears the isMessageAvailable flag.
     */
    RadioMessage readMessage() const override;

    RadioLinkStatus status() const override;

protected:
    uint8_t m_csPin;
    uint8_t m_dio1Pin;
    uint8_t m_resetPin;
    uint8_t m_busyPin;
    uint8_t m_rxEnPin;
    uint8_t m_txEnPin;
    float m_frequencyMHz;

    SX1262 m_radio = nullptr;
    DebugStream* m_debugStream = nullptr;

    RadioLinkStatus m_status = RadioLinkStatus::IDLE;
    mutable RadioMessage m_lastMessage{};
    mutable bool m_messageAvailable = false;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SX1262RADIO_H
