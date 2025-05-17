#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONMEMORY_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONMEMORY_H

class ConfigurationMemory {
public:
    virtual void setup() {};

    virtual bool ready() = 0;

    virtual void write(uint32_t address, const uint8_t* buffer, uint32_t length) = 0;

    virtual void read(uint32_t address, uint8_t* buffer, uint32_t length) = 0;

};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONMEMORY_H
