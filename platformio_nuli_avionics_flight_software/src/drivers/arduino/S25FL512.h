#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_S25FL512_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_S25FL512_H

#include <Avionics.h>
#include <Arduino.h>
#include <SPI.h>

#include "core/generic_hardware/FlashMemory.h"

class S25FL512 final : public FlashMemory {
public:
    explicit S25FL512(uint8_t chipSelectPin);

    void setSpiClass(SPIClass* spiClass);

    void setup() override;

    static uint32_t getMemorySize();

    static uint32_t getSectorSize();

    static uint32_t getSectorNum();

    static uint32_t getPageSize();

    uint32_t getMemorySizeBytes() const override;

    bool ready() const override;

    bool waitForReady(uint32_t timeout) const override;

    void write(uint32_t address, const uint8_t* buffer, uint32_t length, bool waitForCompletion) const override;

    void read(uint32_t address, uint8_t* buffer, uint32_t length) const override;

    uint8_t read(uint32_t address) const override;

    void write(uint32_t address, uint8_t byte) const override;

    void eraseAll(bool waitForCompletion) const override;

    void eraseSector(uint32_t sectorNumber, bool waitForCompletion) const override;

    uint8_t readStatusRegister() const;

protected:
    void pageProgram(uint32_t address, const uint8_t* buffer, uint32_t length) const;

    inline bool waitForWriteCompletion(uint32_t timeout = 1000) const;

    inline bool isWriteInProgress() const;


    inline void enableWrite() const;

    inline void disableWrite() const;

    inline void enableSelectPin() const;

    inline void disableSelectPin() const;

    const uint8_t m_chipSelectPin = 255;
    SPIClass* m_spiBus = &SPI;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_S25FL512_H
