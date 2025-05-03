/*
 * @TODO: Naming
 */

#include "RFM9xRadio.h"
#include "RadioPacketDefinitions.h"

volatile bool operationDone = false;

void setFlagGlobal(void) {
  operationDone = true;
}

void RFM9xRadio::setup() {
  m_radio = new Module(8, 3, 4); // (CS, INT, RST)

  /* just copied from PayloadBoard */
  int state = m_radio.begin(915.0, 125.0, 12);  // (freq, bw, sf)
  m_radio.setOutputPower(20);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("RFM9x Radio initialization successful!"));
  } else {
    Serial.print(F("RFM9x Radio initialization failed. Code: "));
    Serial.println(state);
    return;
  }

  m_radio.setDio0Action(setFlagGlobal, RISING);
  m_radio.startReceive(); // puts the radio into receive mode
}

void RFM9xRadio::loopOnce() {
  if (! operationDone) {
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
      Serial.print(F("RFM9x Radio receive failed. Code: "));
      Serial.println(state);
    }
  } else {
    Serial.print(F("Buffer overflow, dropping data"));
  }
}

bool RFM9xRadio::hasNewData() {
    return m_dataLength > 0;
}

uint32_t RFM9xRadio::getData(uint8_t *data, uint32_t maxLength) {
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

void RFM9xRadio::transmit(uint8_t *data, uint32_t length) {
  int state = m_radio.startTransmit(data, length);

  while (! operationDone);  // @TODO: Interesting?
  operationDone = false;

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("RFM9x Radio: Transmission good"));
  } else {
    Serial.print(F("RFM9x Radio: Transmission failed. Code: "));
    Serial.println(state);
  }

  m_radio.startReceive(); // back to receiving
}
