#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_OTHERCLASSES_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_OTHERCLASSES_H

#include <CommonStructs.h>
#include <CommonHardware.h>
#include <HardwareInterface.h>

class Configuration {
public:
    void setup(HardwareInterface* hardware) {
        m_hardware = hardware;
    }

private:
    HardwareInterface* m_hardware;
};

class Logger {
public:
    void setup(HardwareInterface* hardware, Configuration* configuration) {
        m_hardware = hardware;
        m_configuration = configuration;
    }

    void log() {

    }

private:
    Configuration* m_configuration;
    HardwareInterface* m_hardware;
};

class CommandLineParser {
public:
    void setup(Configuration* configuration, Logger* logger) {
        m_configuration = configuration;
        m_logger = logger;
    }

    Responses parseAndConfigure(Messages* messages) {
        return {};
    }

private:
    Configuration* m_configuration;
    Logger* m_logger;
};

class EventManager {
public:
    void setup(HardwareInterface* hardware, Configuration* configuration, Logger* logger) {
        m_hardware = hardware;
        m_configuration = configuration;
        m_logger = logger;
    }

    ActiveEvents detectEvents(State state, FilteredSensorData* filteredSensorData) {
        return {};
    }

    void executeEvents(ActiveEvents* activeEvents) {

    }

private:
    Configuration* m_configuration;
    HardwareInterface* m_hardware;
    Logger* m_logger;
};

class Filters {
public:
    void setup(Configuration* configuration, Logger* logger) {
        m_configuration = configuration;
        m_logger = logger;
    }

    FilteredSensorData runFilterOnce(RawSensorData* rawSensorData) {
        return {};
    }

private:
    Configuration* m_configuration;
    Logger* m_logger;
};

class StateMachine {
public:
    void setup(Configuration* configuration, Logger* logger) {
        m_configuration = configuration;
        m_logger = logger;
    }

    State updateState(FilteredSensorData* filteredSensorData) {
        return IDLE;
    }

private:
    Configuration* m_configuration;
    Logger* m_logger;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_OTHERCLASSES_H
