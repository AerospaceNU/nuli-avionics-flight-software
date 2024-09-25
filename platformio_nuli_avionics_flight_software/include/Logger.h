#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H

#include <HardwareAbstraction.h>
#include <Configuration.h>

class Logger {
public:
    void setup(HardwareAbstraction* hardware, Configuration* configuration);

    void log();

private:
    Configuration* m_configuration;
    HardwareAbstraction* m_hardware;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H
