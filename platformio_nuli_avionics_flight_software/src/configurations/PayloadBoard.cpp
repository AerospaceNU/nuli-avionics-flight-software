#include <Arduino.h>
#include <Avionics.h>
#include "../src/core/generic_hardware/GenericSensor.h"
#include "../src/core/generic_hardware/Barometer.h"
#include "../src/core/generic_hardware/Accelerometer.h"
#include "../src/core/generic_hardware/GPS.h"
#include "../src/core/generic_hardware/Gyroscope.h"
#include "../src/core/generic_hardware/Magnetometer.h"
#include "../src/core/generic_hardware/Pyro.h"
#include "core/AvionicsCore.h"
#include "core/HardwareAbstraction.h"
#include "core/Filters.h"
#include "drivers/arduino/ICM20948Sensor.h"
#include "drivers/arduino/MS8607Sensor.h"
#include "drivers/arduino/S25FL512.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/USLI2025Payload.h"
#include "drivers/arduino/ArduinoPyro.h"
#include "drivers/arduino/ArduinoVoltageSensor.h"

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
USLI2025Payload payload;
AprsModulation aprsModulation(A0, "KC1UAW");
ArduinoVoltageSensor batterySensor(A4, 22.008);

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
    hardware.addPyro(&pyro1);
    hardware.addVoltageSensor(&batterySensor);
    // Add the ICM20948. This takes multiple steps because the ICM is actually 3 sensors in one
    hardware.addGenericSensor(&icm20948);
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
    avionicsCore.loopOnce();
    cliTick();

    if (operationDone) {
        operationDone = false;

        String str;
        int state = radio.readData(str);

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

                payload.m_transmitAllowed = true;
                radio.startTransmit(payload.getTransmitStr());
                while (!operationDone);
                operationDone = false;
            } else if (str.startsWith("w")) {
                radio.startTransmit("stopping");
                while (!operationDone);
                operationDone = false;

                payload.m_transmitAllowed = false;
            }
        }

        radio.startReceive();
    }
}
