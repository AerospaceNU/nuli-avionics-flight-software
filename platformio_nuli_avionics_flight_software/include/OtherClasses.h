#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_OTHERCLASSES_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_OTHERCLASSES_H

#include <HardwareAbstraction.h>
struct RawSensorData {

};

struct FilteredSensorData {

};

struct TriggeredEvents {

};

struct Messages {
    char* messages[4];
};

enum State {
    IDLE,
    RUNNING,
};
class CommandLineParser {
public:
    void setup(Configuration* configuration, Logger* logger) {
        m_configuration = configuration;
        m_logger = logger;
    }

    Messages parseAndExecute(Messages* messages) {
        return {};
    }

private:
    Configuration* m_configuration;
    Logger* m_logger;
};

class EventManager {
public:
    void setup(HardwareAbstraction* hardware, Configuration* configuration, Logger* logger) {
        m_hardware = hardware;
        m_configuration = configuration;
        m_logger = logger;
    }

    TriggeredEvents detectEvents(State state, FilteredSensorData* filteredSensorData) {
        return {};
    }

//    void executeEvents(ActiveEvents* activeEvents) {
//
//    }

private:
    Configuration* m_configuration;
    HardwareAbstraction* m_hardware;
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
