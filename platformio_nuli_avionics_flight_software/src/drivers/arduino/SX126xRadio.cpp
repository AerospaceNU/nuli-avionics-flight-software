/*
 * @TODO Naming
 */

#include "SX126xRadio.h"
#include "RadioPacketDefinitions.h"

volatile bool operationDone = false;

SX126xRadio::SX126xRadio (
    uint8_t chipSelect,
    uint8_t dio1,
    uint8_t reset,
    uint8_t busy,
    uint8_t rxEnable,
    uint8_t txEnable) {

    //control pins
    m_chipSelect = chipSelect;
    m_dio1 = dio1;
    m_reset = reset;
    m_busy = busy;
    m_rxEnable = rxEnable;
    m_txEnable = txEnable;

    //default radio parameters
    m_frequency = 915.0;
    m_bandwidth = 125.0;
    m_spreadingFactor = 12.0;
}



void setFlagGlobal(void) {
    operationDone = true;
}

void SX126xRadio::setup() {
    m_radio = new Module(m_chipSelect, m_dio1, m_reset, m_busy);
    int state = m_radio.begin(m_frequency, m_bandwidth, m_spreadingFactor);
    m_radio.setOutputPower(20);
    // check for errors
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("SX126x Radio initialization successful!"));
    } else {
        Serial.print(F("SX126x Radio initialization failed. Code: "));
        Serial.println(state);
        return;
    }

    m_radio.setRfSwitchPins(m_rxEnable, m_txEnable);
    m_radio.setDio1Action(setFlagGlobal);  // SX1262 uses DIO1
    // (RadioLib automatically uses BUSY line for SX126x)
    m_radio.startReceive();
}

void SX126xRadio::loopOnce() {
    if (!operationDone) {
        return;
    }
    operationDone = false;

    uint16_t length = m_radio.getPacketLength();
    // check data is within limits
    if (length > 0 && m_dataLength + length <= RADIO_BUFFER_SIZE) {
        uint8_t tempBuffer[RADIO_BUFFER_SIZE];
        int state = m_radio.readData(tempBuffer, length);

        // check state
        if (state == RADIOLIB_ERR_NONE) {
            memcpy(m_inBuffer + m_dataLength, tempBuffer, length);
            m_dataLength += length;
        } else {
            Serial.print(F("SX126x Radio receive failed. Code: "));
            Serial.println(state);
        }
    } else {
        Serial.print(F("Buffer overflow, dropping data"));
    }
}

bool SX126xRadio::hasNewData() {
    return m_dataLength > 0;
}

uint32_t SX126xRadio::getData(uint8_t* data, uint32_t maxLength) {
    uint32_t length = (maxLength < m_dataLength) ? maxLength : m_dataLength;

    // get only the requested length
    memcpy(data, m_inBuffer, length);

    // shift remaining data to the front
    uint32_t remaining = m_dataLength - length;
    if (remaining > 0) {
        memmove(m_inBuffer, m_inBuffer + length, remaining);
    }

    // update length
    m_dataLength = remaining;

    return length;
}

void SX126xRadio::transmit(uint8_t* data, uint32_t length) {
    int state = m_radio.startTransmit(data, length);

    while (!operationDone);  // @TODO: Interesting?
    operationDone = false;

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("SX126x Radio: Transmission good"));
    } else {
        Serial.print(F("SX126x Radio: Transmission failed. Code: "));
        Serial.println(state);
    }

    m_radio.startReceive(); // back to receiving
}

void SX126xRadio::setFrequency(float frequency) {
    m_frequency = frequency;
    m_radio.setFrequency(m_frequency);
}

void SX126xRadio::setBandwidth(float bandwidth) {
    m_bandwidth = bandwidth;
    m_radio.setBandwidth(m_bandwidth);
}

void SX126xRadio::setSpreadingFactor(float spreadingFactor) {
    m_spreadingFactor = spreadingFactor;
    m_radio.setSpreadingFactor(m_spreadingFactor);
}
