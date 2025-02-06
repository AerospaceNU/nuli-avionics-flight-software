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

// Hardware devices
SerialDebug debug;

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




void setup() {
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);
    pinMode(A5, OUTPUT);
    digitalWrite(A5, HIGH);
    Serial.begin(9600);
    while(!Serial);
    SPI.begin();
    Wire.begin();

    Serial.println("Serial successfully started");

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
    avionicsCore.setup(&hardware, &configuration, &logger, &filter);
}


void loop() {
    avionicsCore.loopOnce();
    avionicsCore.printDump();
}
