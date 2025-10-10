#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FLASHMEMORY_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FLASHMEMORY_H

#include <Avionics.h>
#include "core/generic_hardware/DebugStream.h"

class FlashMemory {
public:
    virtual ~FlashMemory() = default;

    virtual void setup(DebugStream *debugStream) {}

    virtual bool ready() const = 0;

    virtual bool waitForReady(uint32_t timeout) const  = 0;

    virtual void write(uint32_t address, const uint8_t *buffer, uint32_t length, bool waitForCompletion) const  = 0;

    virtual void read(uint32_t address, uint8_t *buffer, uint32_t length) const  = 0;

    virtual uint8_t read(uint32_t address) const  = 0;

    virtual void write(uint32_t address, uint8_t byte) const  = 0;

    virtual void eraseAll(bool waitForCompletion) const = 0;

    virtual void eraseSector(uint32_t sectorNumber, bool waitForCompletion) const  = 0;

    virtual uint32_t getMemorySizeBytes() const = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FLASHMEMORY_H
