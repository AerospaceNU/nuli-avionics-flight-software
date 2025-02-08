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
#include "ArduinoSystemClock.h"
#include "SerialDebug.h"
#include "USLI2025Payload.h"

// Hardware devices
SerialDebug debug(false);

ArduinoSystemClock arduinoClock;
MS8607Sensor barometer;
ICM20948Sensor icm20948(5);

// Core objects accessible by all components
HardwareAbstraction hardware;
Configuration configuration;
Logger logger;
// Components, declared here to use dependency injection
Filters filter;

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
    // Finish initializing all hardware
    hardware.setup();
//    logger.setup(&hardware, &configuration);
//    // Initialize components
//    filter.setup(&configuration, &logger);
    // Initialize core
    avionicsCore.setup(&hardware, &configuration, &logger, &filter, &payload);
//    delay(5000);
//    payload.deployLegs();
}


void loop() {
    avionicsCore.loopOnce();
//    avionicsCore.printDump();

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

            if (str.startsWith("d")) {
                payload.deployLegs();
            } else if (str.startsWith("e")) {

            } else if (str.startsWith("l")) {

            } else if(str.startsWith("t")) {
                payload.sendTransmission(millis());
            }
        }

        radio.startReceive();
    }
}
