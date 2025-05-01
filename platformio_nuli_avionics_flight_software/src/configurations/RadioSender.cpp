#include <Arduino.h>
#include "RFM9xRadio.h"
#include "RadioPacketDefinitions.h"
#include "../drivers/arduino/UART_GPS.h"
#include <Adafruit_GPS.h>
#include "../include/cli/Parser.h"
#include "cli/SimpleFlag.h"
#include "CLIEnums.h"
#include "../include/CliRadioManager.h"

#define GPS_SERIAL Serial1
#define MAX_PACKET_SIZE 128

RFM9xRadio radio;
UART_GPS gps(&GPS_SERIAL);

Parser myParser;
uint8_t packetNum = 0;


// Define callback //
/**
 * @brief Handles callbacks from flags.
 * @details Compiles information from flag and sends data over radio
 * @param data Flag specific data
 * @param length Length of the data
 * @param group_uid Which group this flag belongs to
 * @param flag_uid Which flag this data belongs to
 */
void callback(uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid) {
    uint8_t packetBuffer[MAX_PACKET_SIZE];

    // total packet size (header size + data size)
    uint32_t totalPacketSize = sizeof(RadioPacketHeader) + length;

    // handle oversized packets
    if (totalPacketSize > MAX_PACKET_SIZE) {
        return;
    }

    // fill header
    RadioPacketHeader radioPacketHeader{};
    radioPacketHeader.packetNumber = packetNum++;
    radioPacketHeader.packetLength = length;
    radioPacketHeader.groupId = group_uid;
    radioPacketHeader.flagId = flag_uid;

    // copy data in
    memcpy(packetBuffer, &radioPacketHeader, sizeof(RadioPacketHeader));
    memcpy(packetBuffer + sizeof(RadioPacketHeader), data, length);

    radio.transmit(packetBuffer, totalPacketSize);
}

// Define flags //
SimpleFlag start("--start", "Send start", true, 255, callback);
BaseFlag* startGroup[] = {&start};

SimpleFlag stop("--stop", "Send stop", true, 255, callback);
BaseFlag* stopGroup[] = {&stop};


void getSerialInput(char* buffer) {
    if (! Serial.available()) {
        return;
    }

    size_t bytesRead = Serial.readBytes(buffer, 254);
    buffer[bytesRead] = '\0';
}

void sendGPS() {
    uint8_t packetBuffer[MAX_PACKET_SIZE];
    gps.read();

    RadioPacketHeader radioPacketHeader{};
    radioPacketHeader.packetNumber = packetNum++;
    radioPacketHeader.packetLength = sizeof(GPSPacket);
    radioPacketHeader.groupId = GPS;
    radioPacketHeader.flagId = 0;

    GPSPacket gpsPacket{};

    gpsPacket.altitude = gps.getAltitude();
    gpsPacket.HDOP = gps.getHDOP();
    gpsPacket.fixQuality = gps.getFixQuality();
    gpsPacket.latitude = gps.getLatitude();
    gpsPacket.longitude = gps.getLongitude();
    gpsPacket.satellitesTracked = gps.getSatellitesTracked();

    uint32_t totalSize = sizeof(RadioPacketHeader) + sizeof(GPSPacket);

    // copy packets into buffer
    memcpy(packetBuffer, &radioPacketHeader, sizeof(RadioPacketHeader));
    memcpy(packetBuffer + sizeof(RadioPacketHeader), &gpsPacket, sizeof(GPSPacket));

    radio.transmit(packetBuffer, totalSize);
}

void sender() {
    char buf[255];  // @TODO: change into a macro/const
    getSerialInput(buf);

    // parse input
    myParser.parse(buf);
}

// Arduino functions
void setup() {
    radio.setup();
    gps.setup();
    myParser = Parser();

    myParser.addFlagGroup(startGroup, START);
    myParser.addFlagGroup(stopGroup, STOP);
}

void loop() {
    // SENDER LOGIC
    Serial.println("Sending packet");
    sendGPS();
//    sender();
//    myParser.runFlags();
//    myParser.resetFlags();
    //    sender();

    delay(2000);
}