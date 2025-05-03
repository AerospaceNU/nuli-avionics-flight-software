#include <Arduino.h>
#include "RFM9xRadio.h"
#include "RadioPacketDefinitions.h"
#include "../drivers/arduino/UART_GPS.h"
#include "cli/SimpleFlag.h"
#include "CLIEnums.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/MS8607Sensor.h"
#include "drivers/arduino/ICM20948Sensor.h"
#include "drivers/arduino/S25FL512.h"
#include "drivers/arduino/ArduinoPyro.h"
#include "core/HardwareAbstraction.h"
#include "core/Configuration.h"
#include "core/Logger.h"
#include "core/Filters.h"
#include "drivers/arduino/ArduinoVoltageSensor.h"
#include "drivers/arduino/AprsModulation.h"
#include "drivers/arduino/USLI2025Payload.h"
#include "core/AvionicsCore.h"

/*
 *
 * PAYLOAD BOARD
 *
 */

#define GPS_SERIAL Serial1
#define MAX_PACKET_SIZE 128

RFM9xRadio radio;
UART_GPS gps(&GPS_SERIAL);

uint8_t packetNum = 0;

const int BUFFER_SIZE = 16;  // Adjust buffer size as needed
char serialInputBuffer[BUFFER_SIZE];
int bufferIndex = 0;

bool isOffloading = false;
uint32_t currentOffloadAddress = 0;

uint8_t offloadReadBuffer[256];

// Hardware devices
SerialDebug debug(false);

ArduinoSystemClock arduinoClock;
MS8607Sensor barometer;
ICM20948Sensor icm20948(5);
S25FL512 s25fl512(A5);
ArduinoPyro pyro1(11, A1, 75);

// Core objects accessible by all components
HardwareAbstraction hardware;
Configuration configuration;
Logger logger;
// Components, declared here to use dependency injection
Filters filter;

// The core
AvionicsCore avionicsCore;
USLI2025Payload payload;
AprsModulation aprsModulation(A0, "KC1UAW");
ArduinoVoltageSensor batterySensor(A4, 22.008);

void cliTick() {
    while (Serial.available()) {
        char c = Serial.read();
        // Ignore non-printable characters (except newline)
        if (c == '\r') return;

        // Add character to buffer if within size limit
        if (bufferIndex < BUFFER_SIZE - 1) {
            serialInputBuffer[bufferIndex++] = c;
        }

        // Detect end of command (newline or space-based parsing)
        if (c == '\n' || bufferIndex >= BUFFER_SIZE - 1) {
            serialInputBuffer[bufferIndex] = '\0'; // Null-terminate string
            bufferIndex = 0;  // Reset buffer for next message

            // Check if the received string matches "offload"
            if (true || strcmp(serialInputBuffer, "offload") == 0) {
                isOffloading = true;
                currentOffloadAddress = 0;
            }
        }
    }

    while (isOffloading) {
        uint32_t readLength = logger.offloadData(currentOffloadAddress, offloadReadBuffer, sizeof(offloadReadBuffer));
        if (readLength == 0) {
            isOffloading = false;
        } else {
            Serial.write(offloadReadBuffer, readLength);
            delay(5);
        }
        currentOffloadAddress += readLength;
    }
}

/**
 * @brief Send a message via radio with a prefix
 * @param message The message to send
 * @param prefix The prefix to add before the message
 */
void sendStringMessageAck(const char* message, const char* prefix = "Processed flag: ") {
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
    header->isAcknowledgement = true;
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
    Serial.println("Received, sending ACK");
    switch (radioPacketHeader->groupId) {
        case PING:
            Serial.println("Sending ACK for PING");
            sendStringMessageAck("pong");

            break;
        case DEPLOY:
            Serial.println("Sending ACK for DEPLOY");
            sendStringMessageAck("deploying");

            payload.deployLegs();
            break;
        case ERASE:
            Serial.println("Sending ACK for ERASE");
            sendStringMessageAck("erasing");

            logger.erase();
            break;
        case START_LOGGING:
            Serial.println("Sending ACK for START");
            sendStringMessageAck("start logging");

            avionicsCore.log = true;
            break;
        case STOP_LOGGING:
            Serial.println("Sending ACK for STOP");
            sendStringMessageAck("stop logging");

            avionicsCore.log = true;
            break;
        case TRANSMIT:
            Serial.println("Sending ACK for TRANSMIT");
            sendStringMessageAck("transmit");

            payload.sendTransmission(millis());
            payload.m_transmitAllowed = true;
            break;
        case STOPPING:
            sendStringMessageAck("stopping");
            payload.m_transmitAllowed = false;
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
        Serial.println(F("[SX1278 Received Packet!"));
        uint8_t data[RADIO_BUFFER_SIZE];
        uint32_t receivedLength = radio.getData(data, RADIO_BUFFER_SIZE);

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

    // @TODO: COPPIED FROM PAYBOARD.CPP
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);
    pinMode(A5, OUTPUT);
    digitalWrite(A5, HIGH);
    Serial.begin(9600);

    SPI.begin();
    Wire.begin();

    Serial.println("Serial successfully started");

    hardware.setDebugStream(&debug);
    hardware.setSystemClock(&arduinoClock);
//    hardware.addBarometer(&barometer);
    hardware.addPyro(&pyro1);
    hardware.addVoltageSensor(&batterySensor);
    // Add the ICM20948. This takes multiple steps because the ICM is actually 3 sensors in one
//    hardware.addGenericSensor(&icm20948);
    hardware.addAccelerometer(icm20948.getAccelerometer());
    hardware.addGyroscope(icm20948.getGyroscope());
    hardware.addMagnetometer(icm20948.getMagnetometer());
    hardware.addFlashMemory(&s25fl512);
    hardware.addRadioLink(&aprsModulation);
    pinMode(A5, OUTPUT);
    digitalWrite(A5, HIGH);
    digitalWrite(8, HIGH);

    // Finish initializing all hardware
    hardware.setup();
    // Initialize other globals
    configuration.setup();
    payload.setup(&hardware);
    logger.setup(&hardware, &configuration);
    // Initialize components
    filter.setup(&configuration, &logger);
    // Initialize core
    avionicsCore.setup(&hardware, &configuration, &logger, &filter, &payload);
}

void loop() {
    // RECEIVER LOGIC
    Serial.println("Attempting to receive packet");
    receiver();

    avionicsCore.loopOnce();
    cliTick();

    delay(2000);

}