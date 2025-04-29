#include <Arduino.h>
#include "RFM9xRadio.h"
#include "RadioPacketDefinitions.h"
#include "../drivers/arduino/UART_GPS.h"
#include <Adafruit_GPS.h>

RFM9xRadio radio;
// copy and pasted from Ronit_GPS
#define GPS_SERIAL Serial1
UART_GPS gps(&GPS_SERIAL);


void setup() {
    radio.setup();
    gps.setup();
}

void senderGPS() {
    // SENDER LOGIC
    gps.read();

    GPSPacket gpsPacket{};
    gpsPacket.packetType = 0x01;   // Use 0x01 to identify GPS data packets
    gpsPacket.dataLength = sizeof(GPSPacket);

    gpsPacket.altitude = 1.1;
    gpsPacket.HDOP = 2.2;
    gpsPacket.fixQuality = 3;
    gpsPacket.latitude = 4.4;
    gpsPacket.longitude = 5.5;
    gpsPacket.satellitesTracked = 6;


    // Transmit the data
    radio.transmit((uint8_t*)&gpsPacket, sizeof(GPSPacket));
}

void receiverGPS() {
    // Check if there's new data
    radio.loopOnce();
    if (radio.hasNewData()) {
        Serial.println("New data received!");

        // Create a buffer to hold the received struct
        uint8_t receiveBuffer[sizeof(GPSPacket)];

        // Get the data
        uint32_t receivedLength = radio.getData(receiveBuffer, sizeof(receiveBuffer));

        // Verify we received the complete packet
        if (receivedLength == sizeof(GPSPacket)) {
            // Cast the buffer to a GPSPacket struct pointer
            GPSPacket* gpsPacket = (GPSPacket*)receiveBuffer;

            // Verify packet type
            if (gpsPacket->packetType == 0x01) {
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
        } else {
            Serial.print("Incomplete data received. Length: ");
            Serial.println(receivedLength);
        }
    }
}

void loop() {
    // SENDER LOGIC
//    Serial.println("Sending packet");
//    senderGPS();

    // RECEIVER LOGIC
    Serial.println("Attempting to receive packet");
    receiverGPS();


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