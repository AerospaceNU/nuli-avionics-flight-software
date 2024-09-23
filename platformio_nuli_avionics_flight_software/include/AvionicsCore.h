#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CORE_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CORE_H

#include <Avionics.h>
#include <HardwareInterface.h>
#include <Configuration.h>
#include <Logger.h>
#include <OtherClasses.h>
#include <CommonHardware.h>

/**
 * @class Avionics Core
 * @brief Core class that runs the Avionics Arduino codebase
 * @details Plz add details
 */
class AvionicsCore {
public:
    /**
     * @brief Inject hardware, configuration and logger
     * @param hardwareInterface Board specific hardware
     * @param configuration Configuration
     * @param logger Logger
     * @param filter Filters
     * @param parser Command parser
     * @param stateMachine State machine
     * @param eventManager Event manager
     */
    void setup(HardwareInterface* hardwareInterface, Configuration* configuration, Logger* logger, Filters* filter,
               CommandLineParser* parser,
               StateMachine* stateMachine,
               EventManager* eventManager);

    /**
     * @brief Run one tick of the core
     */
    void loopOnce();

private:
    // Global
    HardwareInterface* m_hardware;
    Configuration* m_configuration;
    Logger* m_logger;

    // Local
    Filters* m_filter;
    CommandLineParser* m_parser;
    StateMachine* m_stateMachine;
    EventManager* m_eventManager;

};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CORE_H