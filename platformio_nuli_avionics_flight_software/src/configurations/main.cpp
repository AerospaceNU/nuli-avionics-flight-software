#include <Arduino.h>
#include <HardwareInterface.h>
#include <CommonHardware.h>
#include <CommonStructs.h>
#include <OtherClasses.h>

// Hardware devices
Pyro pyro1(1, A0, 500);
Pyro pyro2(2, A1, 500);
Pyro pyro3(3, A2, Pyro::USE_DIGITAL_CONTINUITY);
Pyro pyro4(4, A3, Pyro::USE_DIGITAL_CONTINUITY);
Barometer barometer;
NineAxisIMU nineAxisIMU;
GPS gps;
FlashMemory flashMemory;
RadioTransmitterLink radioTransmitterLink;
SerialConnectionLink serialConnectionLink;

// Core objects accessible by all components
HardwareInterface hardware;
Configuration configuration;
Logger logger;
// Layer objects
Filters filter;
CommandLineParser parser;
StateMachine stateMachine;
EventManager eventManager;

void setup() {
    // Add all devices to the core
    hardware.addPyro(&pyro1);
    hardware.addPyro(&pyro2);
    hardware.addPyro(&pyro3);
    hardware.addPyro(&pyro4);
    hardware.addBarometer(&barometer);
    hardware.addAccelerometer(nineAxisIMU.getAccelerometer());
    hardware.addGyroscope(nineAxisIMU.getGyroscope());
    hardware.addMagnetometer(nineAxisIMU.getMagnetometer());
    hardware.addGPS(&gps);
    hardware.addFlashMemory(&flashMemory);
    // Communication links
    hardware.addCommunicationLink(&radioTransmitterLink);
    hardware.addCommunicationLink(&serialConnectionLink);
    hardware.setup();       // Finish initializing all hardware
    // Set up the rest of the core
    configuration.setup(&hardware);
    logger.setup(&hardware, &configuration);
    // Initialize all devices
    filter.setup(&configuration, &logger);
    parser.setup(&configuration, &logger);
    stateMachine.setup(&configuration, &logger);
    eventManager.setup(&hardware, &configuration, &logger);
}

/*
 * What I really like, and would not change:
 *      - It's super clear what access the hardware (The logger and configuration will too, but they are abstracted)
 *      - The flow of data is clear: input -> process -> decide -> output
 * Current concerns:
 *      - How is offload handled
 *      - How are multiple communication links handled in messages/responses
 *      - How are multiple sensors and sensors that don't exist handled in RawSensorData
 *      - How are events passed
 */

void loop() {
    // Read in sensor data
    RawSensorData rawSensorData = hardware.readAllSensors();
    Messages receivedMessages = hardware.readCommunicationLinks();
    // Process data to determine outputs
    FilteredSensorData filteredSensorData = filter.runFilterOnce(&rawSensorData);
    State currentState = stateMachine.updateState(&filteredSensorData);
    TriggeredEvents triggeredEvents = eventManager.detectEvents(currentState, &filteredSensorData);
    Messages responses = parser.parseAndExecute(&receivedMessages);
    // Output results
    hardware.writeCommunicationLinks(&responses);
    hardware.executeEvents(&triggeredEvents);
}
