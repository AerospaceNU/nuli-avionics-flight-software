#include <Arduino.h>
#include "RFM9xRadio.h"
#include "RadioPacketDefinitions.h"
#include "../drivers/arduino/UART_GPS.h"
#include <Adafruit_GPS.h>
#include "../include/cli/Parser.h"
#include "cli/SimpleFlag.h"
#include "CLIEnums.h"

#define GPS_SERIAL Serial1
#define MAX_PACKET_SIZE 128

RFM9xRadio radio;
UART_GPS gps(&GPS_SERIAL);

void processGPSPacket(uint8_t* data, uint32_t length) {
    // Verify we received the complete packet
    if (length == sizeof(GPSPacket)) {
        // Cast the buffer to a GPSPacket struct pointer
        GPSPacket* gpsPacket = reinterpret_cast<GPSPacket*>(data);

        // Verify packet type
        Serial.println("Received GPS Packet:");

        // Now you can access the struct fields directly
        Serial.print("Altitude: ");
        Serial.println(gpsPacket->altitude);

        Serial.print("HDOP: ");
        Serial.println(gpsPacket->HDOP);

        Serial.print("Fix Quality: ");
        Serial.println(gpsPacket->fixQuality);

        Serial.print("Latitude: ");
        Serial.println(gpsPacket->latitude);

        Serial.print("Longitude: ");
        Serial.println(gpsPacket->longitude);

        Serial.print("Satellites Tracked: ");
        Serial.println(gpsPacket->satellitesTracked);
    } else {
        Serial.println("Received unknown packet type");
    }
}

void delegatePacket(RadioPacketHeader* radioPacketHeader, uint8_t* subPacketData) {
    switch (radioPacketHeader->groupId) {
        case START:
            Serial.println("Received: START");
            break;
        case STOP:
            Serial.println("Received: STOP");
            break;
        case GPS:
            processGPSPacket(subPacketData, radioPacketHeader->packetLength);
            break;
        default:
            return;
    }
}

void receiver() {
    radio.loopOnce();
    if (radio.hasNewData()) {
        uint8_t data[BUFFER_SIZE];
        uint32_t receivedLength = radio.getData(data, BUFFER_SIZE);

        // ensure header
        if (receivedLength < sizeof(RadioPacketHeader)) {
            return;
        }

        // decode header
        RadioPacketHeader* radioPacketHeader = reinterpret_cast<RadioPacketHeader*>(data);

        // validate packet length
        if (receivedLength != sizeof(RadioPacketHeader) + radioPacketHeader->packetLength) {
            return;
        }

        // pointer to data portion, decode this accordingly
        uint8_t* subPacketData = data + sizeof(RadioPacketHeader);

        // delegate
        delegatePacket(radioPacketHeader, subPacketData);
    }
}

// Arduino functions
void setup() {
    radio.setup();
    gps.setup();
}

void loop() {
    // RECEIVER LOGIC
    Serial.println("Attempting to receive packet");
    receiver();


//    // comment in for sender logic
//    RadioPacketHeader radioPacket;
////  radioPacket.header.startByte = 0xAA;
//    radioPacket.header.packetType = 2;
//    radioPacket.header.packetLength = sizeof(radioPacket);
//    radioPacket.header.packetNumber = 4;
//    radioPacket.header.crc = 16;
//    radioPacket.data = 5;
//
//    uint32_t radioPacketSize = sizeof(radioPacket);
//
////  radio.transmit(reinterpret_cast<uint8_t *>(&radioPacket), radioPacketSize);
////  Serial.println("Sent packet");
//
//
//
//    // comment in for receiver logic
//    radio.loopOnce();
//
//    if (radio.hasNewData()) {
//        Serial.println("Received Data");
//
//        uint8_t data[BUFFER_SIZE];
//        uint32_t length = radio.getData(data, radioPacketSize);
//
//        if (length >= sizeof(HeaderPacket)) {
//            HeaderPacket *header = reinterpret_cast<HeaderPacket *>(data);
//
//            if (header->startByte == 0xAA) {
//                if (length >= radioPacketSize) {
//                    auto *radioPacket1 = reinterpret_cast<RadioPacket *>(data);
//
//                    if (radioPacket1->header.startByte == 0xAA) {
//                        // everything should be received correctly
//                        Serial.print("Packet Number: ");
//                        Serial.println(radioPacket1->header.packetNumber);
//                        Serial.print("Packet Type: ");
//                        Serial.println(radioPacket1->header.packetType);
//                        Serial.print("Packet CRC: ");
//                        Serial.println(radioPacket1->header.crc);
//                        Serial.print("Data is: ");
//                        Serial.println(radioPacket1->data);
//                    } else {
//                        // size is correct, but starting byte does not make sense
//                        for (uint32_t i = 0; i < length; ++i) {
//                            Serial.print(data[i], HEX);
//                            Serial.print(" ");
//                        }
//                        Serial.println();
//                    }
//                } else {
//                    // if unexpected size received
//                    Serial.print("Size incorrect. Length is: ");
//                    Serial.print(length);
//                    Serial.print(" . Expected: ");
//                    Serial.println(radioPacketSize);
//                }
//            }
//        }
//
//    } else {
//        Serial.println("No data received");
//    }
//
    delay(2000);

}