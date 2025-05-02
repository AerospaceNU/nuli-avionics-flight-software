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
void callback(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid) {
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

/**
 * @param name
 * @param data
 * @param length
 * @param group_uid
 * @param flag_uid
 * @note Each callback is provided more information than needed,
 */
void callback_name(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid) {
    // Create a formatted message
    char message[128];  //@TODO Make global constant
    snprintf(message, sizeof(message), "Processed flag: %s", name);

    uint32_t messageLength = strlen(message) + 1;

    uint8_t packetBuffer[MAX_PACKET_SIZE];

    // total packet size (header size + data size)
    uint32_t totalPacketSize = sizeof(RadioPacketHeader) + messageLength;

    // handle oversized packets
    if (totalPacketSize > MAX_PACKET_SIZE) {
        return;
    }

    // fill header
    RadioPacketHeader radioPacketHeader{};
    radioPacketHeader.packetNumber = packetNum++;
    radioPacketHeader.packetLength = messageLength;
    radioPacketHeader.groupId = group_uid;
    radioPacketHeader.flagId = flag_uid;

    // copy data in
    memcpy(packetBuffer, &radioPacketHeader, sizeof(RadioPacketHeader));
    memcpy(packetBuffer + sizeof(RadioPacketHeader), message, messageLength);

    radio.transmit(packetBuffer, totalPacketSize);
}

// Define flags //
SimpleFlag ping("--ping", "Send start", true, 255, callback_name);
BaseFlag* pingGroup[] = {&ping};

SimpleFlag deploy("--deploy", "Send stop", true, 255, callback_name);
BaseFlag* deployGroup[] = {&deploy};

SimpleFlag erase("--erase", "Send stop", true, 255, callback_name);
BaseFlag* eraseGroup[] = {&erase};

SimpleFlag start("--start_logging", "Send stop", true, 255, callback_name);
BaseFlag* startGroup[] = {&start};

SimpleFlag stop("--stop_logging", "Send stop", true, 255, callback_name);
BaseFlag* stopGroup[] = {&stop};

SimpleFlag transmit("--transmit", "Send stop", true, 255, callback_name);
BaseFlag* transmitGroup[] = {&transmit};


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

void serialSender() {
    char buf[255];  // @TODO: change into a macro/const
    getSerialInput(buf);

    // parse input
    myParser.parse(buf);
}

void delegatePacket(RadioPacketHeader* radioPacketHeader, uint8_t* subPacketData) {
    switch (radioPacketHeader->groupId) {
        case PING:
            break;
        case DEPLOY:
            break;
        case ERASE:
            break;
        case START_LOGGING:
            break;
        case STOP_LOGGING:
            break;
        case TRANSMIT:
            break;
        case GPS:
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
    myParser = Parser();

    myParser.addFlagGroup(pingGroup, PING);
    myParser.addFlagGroup(deployGroup, DEPLOY);
    myParser.addFlagGroup(eraseGroup, ERASE);
    myParser.addFlagGroup(startGroup, START_LOGGING);
    myParser.addFlagGroup(stopGroup, STOP_LOGGING);
    myParser.addFlagGroup(transmitGroup, TRANSMIT);
}

void loop() {
    // SENDER LOGIC
    Serial.println("Sending packet");
    sendGPS();
    serialSender();
    myParser.runFlags();
    myParser.resetFlags();

    receiver();

    delay(2000);
}