#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H

#include <HardwareInterface.h>
#include <Configuration.h>

class Logger {
public:
    void setup(HardwareInterface* hardware, Configuration* configuration);

    void log();

private:
    Configuration* m_configuration;
    HardwareInterface* m_hardware;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H
