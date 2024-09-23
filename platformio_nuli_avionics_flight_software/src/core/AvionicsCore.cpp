#include "AvionicsCore.h"

void AvionicsCore::setup(HardwareInterface* hardwareInterface,
                         Configuration* configuration,
                         Logger* logger,
                         Filters* filter,
                         CommandLineParser* parser,
                         StateMachine* stateMachine,
                         EventManager* eventManager) {

    m_hardware = hardwareInterface;
    m_configuration = configuration;
    m_logger = logger;
    m_filter = filter;
    m_parser = parser;
    m_stateMachine = stateMachine;
    m_eventManager = eventManager;
}

void AvionicsCore::loopOnce() {
    // Read in sensor data
    RawSensorData rawSensorData = m_hardware->readAllSensors();
    Messages receivedMessages = m_hardware->readCommunicationLinks();
    // Process data to determine outputs
    FilteredSensorData filteredSensorData = m_filter->runFilterOnce(&rawSensorData);
    State currentState = m_stateMachine->updateState(&filteredSensorData);
    TriggeredEvents triggeredEvents = m_eventManager->detectEvents(currentState, &filteredSensorData);
    Messages responses = m_parser->parseAndExecute(&receivedMessages);
    // Output results
    m_hardware->writeCommunicationLinks(&responses);
    m_hardware->executeEvents(&triggeredEvents);
}


