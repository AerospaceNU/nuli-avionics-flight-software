#include <Arduino.h>
#include "RFM9xRadio.h"
#include "RadioPacketDefinitions.h"

RFM9xRadio radio;

void setup() {
  radio.setup();
}

void loop() {
  // comment in for sender logic
  RadioPacket radioPacket;
//  radioPacket.header.startByte = 0xAA;
  radioPacket.header.packetType = 2;
  radioPacket.header.packetLength = sizeof(radioPacket);
  radioPacket.header.packetNumber = 4;
  radioPacket.header.crc = 16;
  radioPacket.data = 5;

  uint32_t radioPacketSize = sizeof(radioPacket);

//  radio.transmit(reinterpret_cast<uint8_t *>(&radioPacket), radioPacketSize);
//  Serial.println("Sent packet");



  // comment in for receiver logic
  radio.loopOnce();

  if (radio.hasNewData()) {
    Serial.println("Received Data");

    uint8_t data[BUFFER_SIZE];
    uint32_t length = radio.getData(data, radioPacketSize);

    if (length >= sizeof(HeaderPacket)) {
      HeaderPacket *header = reinterpret_cast<HeaderPacket *>(data);

      if (header->startByte == 0xAA) {
        if (length >= radioPacketSize) {
          auto *radioPacket1 = reinterpret_cast<RadioPacket *>(data);

          if (radioPacket1->header.startByte == 0xAA) {
            // everything should be received correctly
            Serial.print("Packet Number: ");
            Serial.println(radioPacket1->header.packetNumber);
            Serial.print("Packet Type: ");
            Serial.println(radioPacket1->header.packetType);
            Serial.print("Packet CRC: ");
            Serial.println(radioPacket1->header.crc);
            Serial.print("Data is: ");
            Serial.println(radioPacket1->data);
          } else {
            // size is correct, but starting byte does not make sense
            for (uint32_t i = 0; i < length; ++i) {
              Serial.print(data[i], HEX);
              Serial.print(" ");
            }
            Serial.println();
          }
        } else {
          // if unexpected size received
          Serial.print("Size incorrect. Length is: ");
          Serial.print(length);
          Serial.print(" . Expected: ");
          Serial.println(radioPacketSize);
        }
      }
    }

  } else {
    Serial.println("No data received");
  }

  delay(2000);

}