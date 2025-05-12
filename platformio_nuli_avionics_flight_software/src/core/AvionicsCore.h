#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CORE_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CORE_H

#include "Avionics.h"
#include "HardwareAbstraction.h"
#include "Configuration.h"
#include "Logger.h"
#include "Filters.h"
#include "../drivers/arduino/USLI2025Payload.h"

/**
 * @class AvionicsCore
 * @brief Core class that runs the Avionics Arduino codebase
 * @details Plz add details
 */
class AvionicsCore {
public:
    bool log = false;
    /**
     * @brief Inject hardware, configuration and logger
     * @param hardware Board specific hardware
     * @param configuration Configuration
     * @param logger Logger
     * @param filter Filters
     * @param parser Command parser
     * @param stateMachine State machine
     * @param eventManager Event manager
     */
    void setup(HardwareAbstraction* hardware, Configuration* configuration, Logger* logger);

    /**
     * @brief Run one tick of the core
     */
    void loopOnce();

    void printDump();

private:
    HardwareAbstraction* m_hardware;
    Configuration* m_configuration;
    Logger* m_logger;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CORE_H
