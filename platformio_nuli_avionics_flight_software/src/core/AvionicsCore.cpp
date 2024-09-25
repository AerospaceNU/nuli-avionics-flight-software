#include "AvionicsCore.h"

void AvionicsCore::setup(HardwareAbstraction* hardware,
                         Configuration* configuration,
                         Logger* logger,
                         Filters* filter,
                         CommandLineParser* parser,
                         StateMachine* stateMachine,
                         EventManager* eventManager) {

    m_hardware = hardware;
    m_configuration = configuration;
    m_logger = logger;
    m_filter = filter;
    m_parser = parser;
    m_stateMachine = stateMachine;
    m_eventManager = eventManager;
}

void AvionicsCore::loopOnce() {
    // Get the start timestamp for this loop
    m_hardware->updateLoopTimestamp();
    // Read in sensor data. This data is accessible through
    m_hardware->readAllSensors();
    m_hardware->readAllCommunicationLinks();



    // Process data to determine outputs
//    FilteredSensorData filteredSensorData = m_filter->runFilterOnce(&rawSensorData);
//    State currentState = m_stateMachine->updateState(&filteredSensorData);
//    TriggeredEvents triggeredEvents = m_eventManager->detectEvents(currentState, &filteredSensorData);
//    Messages responses = m_parser->parseAndExecute(&receivedMessages);
//    // Output results
//    m_hardware->writeCommunicationLinks(&responses);
//    m_hardware->executeEvents(&triggeredEvents);
}


