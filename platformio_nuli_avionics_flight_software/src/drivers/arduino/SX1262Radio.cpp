#include "SX1262Radio.h"

// RadioLib's DIO1 callback must be a plain function pointer, so completion is signalled via this
// flag instead of a member function - safe since only one SX1262Radio is ever instantiated on a board.
static volatile bool s_sx1262OperationDone = false;

static void setSX1262InterruptFlag() {
    s_sx1262OperationDone = true;
}

SX1262Radio::SX1262Radio(uint8_t csPin, uint8_t dio1Pin, uint8_t resetPin, uint8_t busyPin,
                         uint8_t rxEnPin, uint8_t txEnPin, float frequencyMHz) :
    m_csPin(csPin), m_dio1Pin(dio1Pin), m_resetPin(resetPin), m_busyPin(busyPin),
    m_rxEnPin(rxEnPin), m_txEnPin(txEnPin), m_frequencyMHz(frequencyMHz) {}

void SX1262Radio::setup(DebugStream* debugStream, WatchdogTimer* watchdog) {
    m_debugStream = debugStream;

    Module* module = new Module(m_csPin, m_dio1Pin, m_resetPin, m_busyPin);
    m_radio = module;

    // Drive RXEN/TXEN directly rather than through the chip's internal DIO2-as-RF-switch feature -
    // DIO2/DIO3 aren't wired to the MCU on this board, only these two dedicated GPIOs are.
    m_radio.setRfSwitchPins(m_rxEnPin, m_txEnPin);

    // tcxoVoltage defaults to 1.6V, which powers the E22 module's onboard TCXO via DIO3 - without
    // it the module won't get a stable clock reference and RF won't work despite correct wiring.
    const int16_t state = m_radio.begin(m_frequencyMHz);
    if (state != RADIOLIB_ERR_NONE) {
        debugStream->error("SX1262Radio initialization failed. Code: %d", state);
        return;
    }
    m_ok = true;
    module->spiConfig.timeout = 10;
    m_radio.setOutputPower(22); // module's rated max

    m_radio.setDio1Action(setSX1262InterruptFlag);
    startReceive();

    debugStream->message("SX1262Radio initialized");
}

void SX1262Radio::run() {
    if (!m_ok) return;
    if (m_status == RadioLinkStatus::TX_ACTIVE && (int32_t)(millis() - m_txDeadlineMs) > 0) {
        m_debugStream->error("SX1262Radio transmit never finished");
        s_sx1262OperationDone = false;
        if (!startReceive()) m_status = RadioLinkStatus::IDLE;
        return;
    }
    if (!s_sx1262OperationDone) return;
    s_sx1262OperationDone = false;

    if (m_status == RadioLinkStatus::TX_ACTIVE) {
        if (!startReceive()) m_status = RadioLinkStatus::IDLE; // done sending, go back to listening
        return;
    }

    // Otherwise this was a receive completing
    const size_t length = m_radio.getPacketLength();
    if (length > 0 && length <= sizeof(m_lastMessage.data)) {
        const int16_t state = m_radio.readData(m_lastMessage.data, length);
        if (state == RADIOLIB_ERR_NONE) {
            m_lastMessage.length = (uint8_t)length;
            m_lastMessage.rssi = (int16_t)m_radio.getRSSI();
            m_lastMessage.snr = m_radio.getSNR();
            m_messageAvailable = true;
        } else {
            m_debugStream->error("SX1262Radio receive failed. Code: %d", state);
        }
    } else if (length > 0) {
        m_debugStream->error("SX1262Radio dropped oversized packet (%d bytes)", (int)length);
    }

    startReceive(); // re-arm for the next packet
}

bool SX1262Radio::startTransmit(void* data, uint32_t length) {
    // Happens when the transmit interval is shorter than the on-air time (SF11/12: seconds per packet)
    if (!m_ok || m_status == RadioLinkStatus::TX_ACTIVE) return false;

    const int16_t state = m_radio.startTransmit((uint8_t*)data, length);
    if (state != RADIOLIB_ERR_NONE) {
        m_debugStream->error("SX1262Radio transmit failed. Code: %d", state);
        return false;
    }
    m_status = RadioLinkStatus::TX_ACTIVE;
    m_txDeadlineMs = millis() + 2 * (uint32_t)(m_radio.getTimeOnAir(length) / 1000) + 100;
    return true;
}

bool SX1262Radio::startReceive() {
    if (!m_ok) return false;
    const int16_t state = m_radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        m_debugStream->error("SX1262Radio startReceive failed. Code: %d", state);
        return false;
    }
    m_status = RadioLinkStatus::RX_LISTENING;
    return true;
}

bool SX1262Radio::setFrequency(float frequencyMHz) {
    // standby() would abort the transmit without a DIO1 interrupt, leaving m_status stuck at TX_ACTIVE
    if (!m_ok || m_status == RadioLinkStatus::TX_ACTIVE) return false;

    const bool wasReceiving = m_status == RadioLinkStatus::RX_LISTENING || m_status == RadioLinkStatus::RX_ACTIVE;

    m_radio.standby(); // the chip requires standby mode to accept a new frequency
    const int16_t state = m_radio.setFrequency(frequencyMHz);
    if (state != RADIOLIB_ERR_NONE) m_debugStream->error("SX1262Radio setFrequency failed. Code: %d", state);
    if (state == RADIOLIB_ERR_NONE) m_frequencyMHz = frequencyMHz;

    if (wasReceiving) startReceive();
    return state == RADIOLIB_ERR_NONE;
}

bool SX1262Radio::setSpreadingFactor(uint8_t spreadingFactor) {
    // See setFrequency()'s comment - same TX_ACTIVE hazard, same fix.
    if (!m_ok || m_status == RadioLinkStatus::TX_ACTIVE) return false;

    const bool wasReceiving = m_status == RadioLinkStatus::RX_LISTENING || m_status == RadioLinkStatus::RX_ACTIVE;

    m_radio.standby(); // the chip requires standby mode to accept new modulation parameters
    const int16_t state = m_radio.setSpreadingFactor(spreadingFactor);
    if (state != RADIOLIB_ERR_NONE) m_debugStream->error("SX1262Radio setSpreadingFactor failed. Code: %d", state);

    if (wasReceiving) startReceive();
    return state == RADIOLIB_ERR_NONE;
}

bool SX1262Radio::isMessageAvailable() const {
    return m_messageAvailable;
}

RadioLink::RadioMessage SX1262Radio::readMessage() const {
    m_messageAvailable = false;
    return m_lastMessage;
}

RadioLink::RadioLinkStatus SX1262Radio::status() const {
    return m_status;
}
