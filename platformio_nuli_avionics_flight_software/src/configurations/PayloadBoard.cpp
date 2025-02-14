#include <Arduino.h>
#include <Avionics.h>
#include <GenericSensor.h>
#include <Barometer.h>
#include <Accelerometer.h>
#include <GPS.h>
#include <Gyroscope.h>
#include <Magnetometer.h>
#include <Pyro.h>
#include <AvionicsCore.h>
#include <HardwareAbstraction.h>
#include <Filters.h>
#include "ICM20948Sensor.h"
#include "MS8607Sensor.h"
#include "S25FL512.h"
#include "ArduinoSystemClock.h"
#include "SerialDebug.h"
#include "USLI2025Payload.h"

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

// Core objects accessible by all components
HardwareAbstraction hardware;
Configuration configuration;
Logger logger;
// Components, declared here to use dependency injection
Filters filter;

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


// The core
AvionicsCore avionicsCore;
USLI2025Payload payload("KC1UAW");

/**
 * Payload logging requirements:
 *
 * Barometer: 3
 * IMU: 9
 * Clock: 1
 * GPS: 3
 * Battery: 1
 * Pyro: 1
 *
 * Lets just log one page at a time
 */
#include <RadioLib.h>

SX1276 radio = new Module(8, 3, 4);  // (CS, INT, RST)

volatile bool operationDone = false;

void setFlag() {
    operationDone = true; // we sent or received  packet, set the flag
}

void setup() {
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);
    pinMode(A5, OUTPUT);
    digitalWrite(A5, HIGH);
    Serial.begin(9600);
//    while(!Serial);
    SPI.begin();
    Wire.begin();
    payload.setup();

    Serial.println("Serial successfully started");

    int state = radio.begin(905.0, 125.0, 12);
    radio.setOutputPower(20);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
    }
    radio.setDio0Action(setFlag, RISING);
    radio.startReceive();


    hardware.setDebugStream(&debug);
    hardware.setSystemClock(&arduinoClock);
    hardware.addBarometer(&barometer);
    // Add the ICM20948. This takes multiple steps because the ICM is actually 3 sensors in one
    hardware.addGenericSensor(&icm20948);
    hardware.addAccelerometer(icm20948.getAccelerometer());
    hardware.addGyroscope(icm20948.getGyroscope());
    hardware.addMagnetometer(icm20948.getMagnetometer());
    hardware.addFlashMemory(&s25fl512);
    pinMode(A5, OUTPUT);
    digitalWrite(A5, HIGH);
    digitalWrite(8, HIGH);

//     s25fl512.eraseAll();
    // Serial.println("erase complete");


    // Finish initializing all hardware
    hardware.setup();
    // Initialize other globals
    configuration.setup();

    logger.setup(&hardware, &configuration);
    // Initialize components
    filter.setup(&configuration, &logger);
    // Initialize core
    avionicsCore.setup(&hardware, &configuration, &logger, &filter, &payload);

}


void loop() {
    avionicsCore.loopOnce();
    cliTick();

    if (operationDone) {
        operationDone = false;

        String str;
        int state = radio.readData(str);

        Serial.println("fads");

        if (state == RADIOLIB_ERR_NONE) {
            // packet was successfully received
            Serial.println(F("[SX1278] Received packet!"));

            // print data of the packet
            Serial.print(F("[SX1278] Data:\t\t"));
            Serial.println(str);

            if (str.startsWith("p")) {
                radio.startTransmit("pong");
                while (!operationDone);
                operationDone = false;
            } else if (str.startsWith("d")) {
                radio.startTransmit("deploying");
                while (!operationDone);
                operationDone = false;
                payload.deployLegs();
            } else if (str.startsWith("e")) {
                radio.startTransmit("erasing");
                while (!operationDone);
                operationDone = false;
                logger.erase();
            } else if (str.startsWith("l")) {
                radio.startTransmit("started logging");
                while (!operationDone);
                operationDone = false;
                avionicsCore.log = true;
            } else if (str.startsWith("s")) {
                radio.startTransmit("stopped logging");
                while (!operationDone);
                operationDone = false;
                avionicsCore.log = false;
            } else if (str.startsWith("t")) {
                radio.startTransmit("transmitting");
                while (!operationDone);
                operationDone = false;
                payload.sendTransmission(millis());
                radio.startTransmit(payload.getTransmitStr());
                while (!operationDone);
            }
        }

        radio.startReceive();
    }
}
