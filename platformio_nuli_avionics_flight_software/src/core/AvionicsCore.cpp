#include "AvionicsCore.h"

void AvionicsCore::setup(HardwareAbstraction* hardware,
                         Configuration* configuration,
                         Logger* logger,
                         Filters* filter) {

    m_hardware = hardware;
    m_configuration = configuration;
    m_logger = logger;
    m_filter = filter;

    double groundLevelStorage = 123.21;
    int groundLevelId = m_configuration->newConfigurable("test", &groundLevelStorage);

    const int * result = m_configuration->getPtr<int>(groundLevelId);
    int result2 = m_configuration->get<int>(groundLevelId);
    m_configuration->set<int>(groundLevelId, 5);

}

void AvionicsCore::loopOnce() {
    // Get the start timestamp for this loop
    m_hardware->updateLoopTimestamp();
    // Read in sensor data. This data is accessible through
    m_hardware->readAllSensors();
    m_hardware->readAllCommunicationLinks();
    // Filter sensors
    m_filter->runFilterOnce();
    // End by pushing configuration updates to flash
    m_configuration->writeFlashIfUpdated();
}


