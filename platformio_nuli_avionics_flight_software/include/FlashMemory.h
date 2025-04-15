#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FLASHMEMORY_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FLASHMEMORY_H

#include <Avionics.h>

class FlashMemory {
public:
    virtual void setup() {}

    virtual bool ready() const = 0;

    virtual bool waitForReady(uint32_t timeout = 1000) const  = 0;

    virtual void write(uint32_t address, const uint8_t *buffer, uint32_t length, bool waitForCompletion = true) const  = 0;

    virtual void read(uint32_t address, uint8_t *buffer, uint32_t length) const  = 0;

    virtual uint8_t read(uint32_t address) const  = 0;

    virtual void write(uint32_t address, uint8_t byte) const  = 0;

    virtual void eraseAll(bool waitForCompletion = true) const = 0;

    virtual void eraseSector(uint32_t sectorNumber, bool waitForCompletion = true) const  = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FLASHMEMORY_H
