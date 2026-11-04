#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FLASHMEMORYCOMMON_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FLASHMEMORYCOMMON_H

#include <Avionics.h>
#include <Arduino.h>
#include <SPI.h>
#include "core/generic_hardware/GenericHardware.h"

// Device-specific data
struct FlashMemoryData_s {
    const uint32_t eraseAllTime;
    const uint32_t sectorSize;
    const uint32_t memorySize;
    const uint32_t pageSize;
};

class FlashMemoryCommon : public FlashMemory {
public:
    FlashMemoryCommon(uint8_t chipSelectPin, const FlashMemoryData_s& deviceData);

    virtual ~FlashMemoryCommon() = 0;

    void setSpiClass(SPIClass* spiClass);

    void setup(DebugStream* debugStream, WatchdogTimer* watchdog) override;

    void run() override;

    uint32_t getMemorySize() const;

    uint32_t getSectorSize() const;

    uint32_t getSectorNum() const;

    uint32_t getPageSize() const;

    uint32_t getMemorySizeBytes() const override;

    bool ready() const override;

    bool waitForReady(uint32_t timeout) const override;

    void write(uint32_t address, const uint8_t* buffer, uint32_t length) const override;

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

    // Drains the whole buffer; may block between page-programs, so only for rare one-shot flushes
    void flushBuffer() const;

    // Programs up to one page if the chip is idle, else skips; a page-program (few ms) finishes before the next tick
    void writeBufferedIfReady() const;

    bool programAndShiftBuffer(uint32_t pageBytes) const;

    const uint8_t m_chipSelectPin;
    const FlashMemoryData_s& m_deviceData;
    SPIClass* m_spiBus = &SPI;
    WatchdogTimer* m_watchdog = nullptr;
    DebugStream* m_debugStream = nullptr;

    static constexpr uint32_t MAX_PAGE_SIZE = 512; // >= every FlashMemoryData_s::pageSize in use
    static constexpr uint32_t PAGE_BUFFER_CAPACITY = 2048; // holds a whole logConfig() burst (~508 B) so write() never flushes mid-burst
    mutable uint8_t m_pageBuffer[PAGE_BUFFER_CAPACITY];
    mutable uint32_t m_bufferedAddress = 0;
    mutable uint32_t m_bufferedLength = 0;
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FLASHMEMORYCOMMON_H
