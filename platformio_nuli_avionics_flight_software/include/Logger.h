#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H

#include <HardwareAbstraction.h>
#include <Configuration.h>

class Logger {
public:
    void setup(HardwareAbstraction* hardware, Configuration* configuration);

    void log();

    void erase();

    uint32_t offloadData(uint32_t readAddress, uint8_t* buffer, const uint32_t length);

private:
    Configuration* m_configuration;
    HardwareAbstraction* m_hardware;
    uint32_t m_logWriteAddress;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H
