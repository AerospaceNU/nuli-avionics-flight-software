#include <Arduino.h>
#include <Avionics.h>
#include <AvionicsCore.h>
#include <HardwareAbstraction.h>
#include <CommonHardware.h>
#include <CommonStructs.h>
#include <OtherClasses.h>

// Hardware devices
Pyro pyro1(1, A0, 500);
Pyro pyro2(2, A1, Pyro::USE_DIGITAL_CONTINUITY);
Barometer barometer;
NineAxisIMU nineAxisIMU;
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
    hardware.addAccelerometer(nineAxisIMU.getAccelerometer());
    hardware.addGyroscope(nineAxisIMU.getGyroscope());
    hardware.addMagnetometer(nineAxisIMU.getMagnetometer());
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
//    avionicsCore.loopOnce();
}

/*
 * What I really like, and would not change:
 *      - It's super clear what access the hardware (The logger and configuration will too, but they are abstracted)
 *      - The flow of data is clear: input -> process -> decide -> output
 * Current concerns:
 *      - Performance: there is virtual methods and copying here
 *      - How is offload handled
 *      - How are multiple communication links handled in messages/responses
 *      - How are multiple sensors and sensors that don't exist handled in RawSensorData
 *          - Fixed vs dynamic QTY?
 *      - How are events passed
 *          - Index mapping?
 *          - Standard types?
 */
