#include "SX1262Radio.h"

// RadioLib's DIO1 callback is a plain function pointer; only one SX1262Radio exists per board
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

    m_radio = new Module(m_csPin, m_dio1Pin, m_resetPin, m_busyPin);

    // DIO2 isn't usable as the RF switch on this board, so RXEN/TXEN are driven as GPIOs
    m_radio.setRfSwitchPins(m_rxEnPin, m_txEnPin);

    // Default tcxoVoltage (1.6 V) powers the E22's TCXO via DIO3; RF doesn't work without it
    const int16_t state = m_radio.begin(m_frequencyMHz);
    if (state != RADIOLIB_ERR_NONE) {
        debugStream->error("SX1262Radio initialization failed. Code: %d", state);
        return;
    }
    m_radio.setOutputPower(22); // module's rated max

    m_radio.setDio1Action(setSX1262InterruptFlag);
    startReceive();

    debugStream->message("SX1262Radio initialized");
}

void SX1262Radio::run() {
    if (!s_sx1262OperationDone) return;
    s_sx1262OperationDone = false;

    if (m_status == RadioLinkStatus::TX_ACTIVE) {
        startReceive();
        return;
    }

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

    startReceive();
}

bool SX1262Radio::startTransmit(void* data, uint32_t length) {
    // Happens when the transmit interval is shorter than the on-air time (SF11/12: seconds per packet)
    if (m_status == RadioLinkStatus::TX_ACTIVE) return false;

    const int16_t state = m_radio.startTransmit((uint8_t*)data, length);
    if (state != RADIOLIB_ERR_NONE) {
        m_debugStream->error("SX1262Radio transmit failed. Code: %d", state);
        return false;
    }
    m_status = RadioLinkStatus::TX_ACTIVE;
    return true;
}

bool SX1262Radio::startReceive() {
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
    if (m_status == RadioLinkStatus::TX_ACTIVE) return false;

    const bool wasReceiving = m_status == RadioLinkStatus::RX_LISTENING || m_status == RadioLinkStatus::RX_ACTIVE;

    m_radio.standby(); // required to accept a new frequency
    const int16_t state = m_radio.setFrequency(frequencyMHz);
    if (state != RADIOLIB_ERR_NONE) m_debugStream->error("SX1262Radio setFrequency failed. Code: %d", state);
    if (state == RADIOLIB_ERR_NONE) m_frequencyMHz = frequencyMHz;

    if (wasReceiving) startReceive();
    return state == RADIOLIB_ERR_NONE;
}

bool SX1262Radio::setSpreadingFactor(uint8_t spreadingFactor) {
    // Same TX_ACTIVE hazard as setFrequency()
    if (m_status == RadioLinkStatus::TX_ACTIVE) return false;

    const bool wasReceiving = m_status == RadioLinkStatus::RX_LISTENING || m_status == RadioLinkStatus::RX_ACTIVE;

    m_radio.standby(); // required to accept new modulation parameters
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
