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

    // Writes out whatever's currently buffered (capped to one physical page) every tick, if the chip's
    // ready (registered hardware gets run() called automatically by HardwareAbstraction) - see
    // writeBufferedIfReady() for why this never blocks a normal tick.
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

    // Fully drains the write buffer (see m_pageBuffer below), however many page-programs that takes -
    // used where an immediate, complete flush is required (flush(), and the non-contiguous-write case
    // in write()), unlike writeBufferedIfReady()'s one-page-per-call pacing. Unlike that one, this can
    // block on waitForReady() if a multi-page backlog needs more than one program - acceptable here
    // since both callers are rare, one-shot, ground/landing events, never the flight-critical tick.
    void flushBuffer() const;

    // Writes out whatever's currently buffered (capped to one physical page) if the chip's ready right
    // now - called every tick from run(). Never blocks: skips entirely (tries again next tick) rather
    // than waiting, if the chip's still busy. A page-program (low-single-digit ms, worst case, per its
    // datasheet) always finishes well within the ~10ms before the next tick's call, so in practice this
    // succeeds essentially every tick there's anything buffered - keeping data durable within about one
    // tick of being written, at any logging rate, without a separate idle-timeout mechanism.
    void writeBufferedIfReady() const;

    // Programs exactly pageBytes buffered bytes (never more than one physical page's worth, starting
    // at m_bufferedAddress) and shifts any bytes left in m_pageBuffer down to the front - the shared
    // step both flushBuffer() (loops this until empty) and writeBufferedIfReady() (calls it once) use.
    void programAndShiftBuffer(uint32_t pageBytes) const;

    const uint8_t m_chipSelectPin;
    const FlashMemoryData_s& m_deviceData;
    SPIClass* m_spiBus = &SPI;
    WatchdogTimer* m_watchdog = nullptr;
    DebugStream* m_debugStream = nullptr;

    // write() coalesces records into pages, so a record straddling a page boundary never forces a
    // mid-write wait; bytes flush in address order, so a record's last byte is committed last - a
    // reset dropping the buffered tail leaves that record's start valid but its tail reading back as
    // stale flash.
    static constexpr uint32_t MAX_PAGE_SIZE = 512; // >= every FlashMemoryData_s::pageSize in use - one page-program's worth, not the buffer's total capacity below
    // Bigger than one page so a single burst write (logConfig() logs ~8 + MAX_CONFIGURATION_LENGTH
    // bytes back to back - see BasicLogger.h) never forces write() to flush mid-burst; it just
    // accumulates here and drains at a steady one page per tick via writeBufferedIfReady() instead.
    static constexpr uint32_t PAGE_BUFFER_CAPACITY = 2048; // comfortable margin above the ~508-byte worst-case logConfig() burst plus a leftover entry
    mutable uint8_t m_pageBuffer[PAGE_BUFFER_CAPACITY];
    mutable uint32_t m_bufferedAddress = 0;
    mutable uint32_t m_bufferedLength = 0;
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FLASHMEMORYCOMMON_H
