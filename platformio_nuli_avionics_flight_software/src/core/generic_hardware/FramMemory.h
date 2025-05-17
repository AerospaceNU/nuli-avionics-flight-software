#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FRAMMEMORY_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FRAMMEMORY_H

class FramMemory {
public:
    virtual void setup() {};

    virtual bool ready() = 0;

    virtual void write(uint32_t address, const uint8_t* buffer, uint32_t length) = 0;

    virtual void read(uint32_t address, uint8_t* buffer, uint32_t length) = 0;

};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FRAMMEMORY_H
