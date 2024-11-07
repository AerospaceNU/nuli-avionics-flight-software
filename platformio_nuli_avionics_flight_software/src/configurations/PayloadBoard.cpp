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
#include <FlashMemory.h>
#include <CommunicationLink.h>
#include "drivers/arduino/icm20948/ICM20948.h"

// Hardware devices
//Pyro pyro1(1, A0, 500);
//Pyro pyro2(2, A1, Pyro::USE_DIGITAL_CONTINUITY);
//Barometer barometer;
ICM20948 icm20948(5);
//GPS gps(9600);
//FlashMemory flashMemory;
//RadioTransmitterLink radioTransmitterLink;
//SerialConnectionLink serialConnectionLink;

// Core objects accessible by all components
HardwareAbstraction hardware;
Configuration configuration;
Logger logger;
// Components, declared here to use dependency injection
Filters filter;

// The core
AvionicsCore avionicsCore;

void setup() {
    // Arduino setup
//    SPI.begin();
//    pinMode(8, OUTPUT);
//    digitalWrite(8, HIGH);
    // Add all hardware
//    hardware.addPyro(&pyro1);
//    hardware.addPyro(&pyro2);
//    hardware.addBarometer(&barometer);
//    hardware.addGPS(&gps);
//    hardware.addFlashMemory(&flashMemory);
//    hardware.addCommunicationLink(&radioTransmitterLink);
//    hardware.addCommunicationLink(&serialConnectionLink);
    // Add the ICM20948. This takes multiple steps because the ICM is actually 3 sensors in one
    hardware.addGenericSensor(&icm20948);
    hardware.addAccelerometer(icm20948.getAccelerometer());
    hardware.addGyroscope(icm20948.getGyroscope());
    hardware.addMagnetometer(icm20948.getMagnetometer());
    // Finish initializing all hardware
    hardware.setup();
    // Initialize other globals
    configuration.setup(&hardware);
    logger.setup(&hardware, &configuration);
    // Initialize components
    filter.setup(&configuration, &logger);
    // Initialize core
    avionicsCore.setup(&hardware, &configuration, &logger, &filter);
}

void loop() {
    avionicsCore.loopOnce();
}
