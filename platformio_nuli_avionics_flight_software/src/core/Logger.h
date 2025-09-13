#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H

#include "HardwareAbstraction.h"
#include "Configuration.h"
#include "cli/Parser.h"

class Logger {
public:
    constexpr static ConfigurationID_e REQUIRED_CONFIGS[] = {FLASH_START_LOC};

    void setup(HardwareAbstraction* hardware, Parser* parser, uint8_t id);

    void log();

    void erase();

    uint32_t offloadData(uint32_t readAddress, uint8_t* buffer, uint32_t length) const;

private:
    HardwareAbstraction* m_hardware = nullptr;
    FlashMemory* m_flash = nullptr;
    uint32_t m_logWriteAddress = 0;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LOGGER_H