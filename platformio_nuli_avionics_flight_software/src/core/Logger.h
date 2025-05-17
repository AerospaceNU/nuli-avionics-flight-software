#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H

#include "HardwareAbstraction.h"
#include "Configuration.h"

class Logger {
public:
    constexpr static ConfigurationID_e REQUIRED_CONFIGS[] = {FLASH_START_LOC};

    void setup(HardwareAbstraction* hardware);

    void log();

    void erase();

    uint32_t offloadData(uint32_t readAddress, uint8_t* buffer, const uint32_t length);

private:
    HardwareAbstraction* m_hardware;
    uint32_t m_logWriteAddress;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H