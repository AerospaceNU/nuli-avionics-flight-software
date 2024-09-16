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
RadioTransmitter radioTransmitter;
SerialConnection serialConnection;

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
    hardware.addCommunicationLink(&radioTransmitter);
    hardware.addCommunicationLink(&serialConnection);
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

void loop() {
    // Input Layer
    RawSensorData rawSensorData = hardware.readAllSensors();
    Messages messages = hardware.readAllMessages();
    // Processing Layer
    FilteredSensorData filteredSensorData = filter.runFilterOnce(&rawSensorData);
    Responses responses = parser.parseAndConfigure(&messages);
    // Event Layer
    State currentState = stateMachine.updateState(&filteredSensorData);
    ActiveEvents activeEvents = eventManager.detectEvents(currentState, &filteredSensorData);
    // Action Layer
    eventManager.executeEvents(&activeEvents);
    hardware.sendResponses(&responses);
}