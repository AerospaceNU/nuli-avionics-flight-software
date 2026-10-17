#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SX1262RADIO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SX1262RADIO_H

#include "core/generic_hardware/GenericHardware.h"
#include <RadioLib.h>

/** @brief RadioLib SX1262 driver (SeriousGoose: EBYTE E22-900MM22S) */
class SX1262Radio : public RadioLink {
public:
    /** @param dio1Pin DIO1 interrupt pin - fires on TX done / RX done */
    SX1262Radio(uint8_t csPin, uint8_t dio1Pin, uint8_t resetPin, uint8_t busyPin,
                uint8_t rxEnPin, uint8_t txEnPin, float frequencyMHz);

    void setup(DebugStream* debugStream, WatchdogTimer* watchdog) override;

    /** @brief Services the TX/RX completion flagged by DIO1, then re-arms receive */
    void run() override;

    /** @brief Non-blocking; completion is picked up by run() */
    bool startTransmit(void* data, uint32_t length) override;

    bool startReceive() override;

    /** @brief Rejected while transmitting; resumes receive afterward if it was receiving */
    bool setFrequency(float frequencyMHz) override;

    /** @brief Spreading factor 5-12; same TX/RX handling as setFrequency() */
    bool setSpreadingFactor(uint8_t spreadingFactor) override;

    bool isMessageAvailable() const override;

    /** @brief Clears isMessageAvailable() */
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
