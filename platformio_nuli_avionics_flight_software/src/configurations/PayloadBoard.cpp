#include <Arduino.h>
#include <Avionics.h>
#include <BaseSensor.h>
#include <Barometer.h>
#include <Accelerometer.h>
#include <GPS.h>
#include <Gyroscope.h>
#include <Magnetometer.h>
#include <Pyro.h>
#include <AvionicsCore.h>
#include <HardwareAbstraction.h>
#include <OtherClasses.h>
#include <FlashMemory.h>
#include <CommunicationLink.h>

// Hardware devices
Pyro pyro1(1, A0, 500);
Pyro pyro2(2, A1, Pyro::USE_DIGITAL_CONTINUITY);
Barometer barometer;
Accelerometer accelerometer;
Magnetometer magnetometer;
Gyroscope gyroscope;
GPS gps;
FlashMemory flashMemory;
RadioTransmitterLink radioTransmitterLink;
SerialConnectionLink serialConnectionLink;

// Core objects accessible by all components
HardwareAbstraction hardware;
Configuration configuration;
Logger logger;
// Components, declared here to use dependency injection
Filters filter;
CommandLineParser parser;
StateMachine stateMachine;
EventManager eventManager;

// The core
AvionicsCore avionicsCore;

void setup() {
    // Add all hardware
    hardware.addPyro(&pyro1);
    hardware.addPyro(&pyro2);
    hardware.addBarometer(&barometer);
    hardware.addAccelerometer(&accelerometer);
    hardware.addGyroscope(&gyroscope);
    hardware.addMagnetometer(&magnetometer);
    hardware.addGPS(&gps);
    hardware.addFlashMemory(&flashMemory);
    hardware.addCommunicationLink(&radioTransmitterLink);
    hardware.addCommunicationLink(&serialConnectionLink);
    // Initialize globals
    hardware.setup();       // Finish initializing all hardware
    configuration.setup(&hardware);
    logger.setup(&hardware, &configuration);
    // Initialize components
    filter.setup(&configuration, &logger);
    parser.setup(&configuration, &logger);
    stateMachine.setup(&configuration, &logger);
    eventManager.setup(&hardware, &configuration, &logger);
    // Initialize core
    avionicsCore.setup(&hardware, &configuration, &logger, &filter, &parser, &stateMachine, &eventManager);
}

void loop() {
    avionicsCore.loopOnce();
}
