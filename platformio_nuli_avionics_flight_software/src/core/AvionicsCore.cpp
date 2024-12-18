#include "AvionicsCore.h"

void AvionicsCore::setup(HardwareAbstraction* hardware,
                         Configuration* configuration,
                         Logger* logger,
                         Filters* filter) {

    m_hardware = hardware;
    m_configuration = configuration;
    m_logger = logger;
    m_filter = filter;
}

void AvionicsCore::loopOnce() {
    // Get the start timestamp for this loop
    m_hardware->updateLoopTimestamp();
    // Read in sensor data. This data is accessible through
    m_hardware->readAllSensors();
    m_hardware->readAllCommunicationLinks();
    // Filter sensors
    m_filter->runFilterOnce();
    // ^ sensorData = m_filter->runFilterOnce();
    // state = updateState();
    // for i in detectors
    //    detector.detect()

    // for i in outputs
    //


    // End by pushing configuration updates to flash
    m_configuration->writeFlashIfUpdated();
}


