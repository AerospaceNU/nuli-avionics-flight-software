#include <Arduino.h>
#include "RFM9xRadio.h"
#include "RadioPacketDefinitions.h"
#include "../drivers/arduino/UART_GPS.h"
#include "../include/cli/Parser.h"
#include "cli/SimpleFlag.h"
#include "CLIEnums.h"

#define GPS_SERIAL Serial1
#define MAX_PACKET_SIZE 128

RFM9xRadio radio;
UART_GPS gps(&GPS_SERIAL);

uint8_t packetNum = 0;

/*
 *
 * PAYLOAD BOARD
 *
 */

/**
 * @brief Send a message via radio with a prefix
 * @param message The message to send
 * @param prefix The prefix to add before the message
 */
void sendStringMessage(const char* message, const char* prefix = "Processed flag: ") {
    // calculate lengths
    size_t prefixLen = strlen(prefix);
    size_t messageLen = strlen(message);
    size_t combinedMsgLen = prefixLen + messageLen + 1;

    // total packet size
    size_t totalPacketSize = sizeof(RadioPacketHeader) + combinedMsgLen;

    uint8_t buffer[MAX_PACKET_SIZE];

    // handle oversized packets
    if (totalPacketSize > MAX_PACKET_SIZE) {
        return;
    }

    // fill header
    RadioPacketHeader* header = (RadioPacketHeader*)buffer;
    header->packetNumber = packetNum++;
    header->packetLength = combinedMsgLen;
    header->groupId = STRING;
    header->flagId = STRING;

    // append string message
    char* msgPtr = (char*)(buffer + sizeof(RadioPacketHeader));
    strcpy(msgPtr, prefix);
    strcpy(msgPtr + prefixLen, message);

    radio.transmit(buffer, totalPacketSize);
}

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
        case PING:
            sendStringMessage("pong");
            break;
        case DEPLOY:
            sendStringMessage("deploying");
            break;
        case ERASE:
            sendStringMessage("erasing");
            break;
        case START_LOGGING:
            sendStringMessage("start logging");
            break;
        case STOP_LOGGING:
            sendStringMessage("stop logging");
            break;
        case TRANSMIT:
            sendStringMessage("transmit");
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

    delay(2000);

}