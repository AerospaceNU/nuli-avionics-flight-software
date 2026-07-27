#include "FlashMemoryCommon.h"
#include <Arduino.h>
#include <SPI.h>

#define DISABLE_WRITE_CMD 0x04
#define ENABLE_WRITE_CMD 0x06
#define CHIP_ERASE_CMD 0xC7
#define SECTOR_ERASE_CMD 0xDC
#define READ_4BYTE_CMD 0x13
#define PAGE_PROGRAM_CMD 0x12

#define STATUS_CMD 0x05
#define STATUS_WRITE_IN_PROGRESS_BIT 0x01

#define CLOCK_SPI_DATA 0x00

// 12MHz: both flash chips support 100MHz+, but this is Adafruit's tested practical ceiling for the
// SAMD21 SPI peripheral - register math allows 24MHz (48MHz ref / 2), untested for signal integrity.
static constexpr uint32_t FLASH_SPI_CLOCK_HZ = 12000000;

/**
 * @todo Figure out where we wait for completion
 * @todo Figure out when we call write disable
 * @todo Standardize API
 */

FlashMemoryCommon::FlashMemoryCommon(const uint8_t chipSelectPin,
                                     const FlashMemoryData_s& deviceData)
    : m_chipSelectPin(chipSelectPin), m_deviceData(deviceData) {}

FlashMemoryCommon::~FlashMemoryCommon() = default;

void FlashMemoryCommon::setSpiClass(SPIClass* spiClass) {
    m_spiBus = spiClass;
}

void FlashMemoryCommon::setup(DebugStream* debugStream, WatchdogTimer* watchdog) {
    m_watchdog = watchdog;
    m_debugStream = debugStream;
    // @todo check a register to make sure the device is there
    m_spiBus->begin();
    pinMode(m_chipSelectPin, OUTPUT);
    disableSelectPin();
    if (m_deviceData.pageSize > MAX_PAGE_SIZE) {
        debugStream->error("Flash page size exceeds MAX_PAGE_SIZE - the page-boundary math assumes every real page fits in one PAGE_BUFFER_CAPACITY-sized buffer");
    }
    debugStream->message("FlashMemoryCommon initialized");
}


uint32_t FlashMemoryCommon::getMemorySize() const {
    return m_deviceData.memorySize;
}

uint32_t FlashMemoryCommon::getSectorSize() const {
    return m_deviceData.sectorSize;
}

uint32_t FlashMemoryCommon::getSectorNum() const {
    return m_deviceData.memorySize / m_deviceData.sectorSize;
}

uint32_t FlashMemoryCommon::getPageSize() const {
    return m_deviceData.pageSize;
}


bool FlashMemoryCommon::ready() const {
    // Was isWriteInProgress() directly (true while busy) despite the name, so every "if (!ready())"
    // call site was backwards. Fixed - no external callers depended on the old meaning.
    return !isWriteInProgress();
}


void FlashMemoryCommon::write(uint32_t address, const uint8_t* buffer, uint32_t length) const {
    // Non-contiguous with what's buffered (shouldn't happen for BasicLogger, but handled safely):
    // flush the old data under its own address before starting fresh.
    if (m_bufferedLength > 0 && address != m_bufferedAddress + m_bufferedLength) {
        flushBuffer();
    }
    if (m_bufferedLength == 0) {
        m_bufferedAddress = address;
    }

    // Shouldn't happen given PAGE_BUFFER_CAPACITY's sizing (a single write() call is always at most
    // one log entry, far smaller), but a caller writing enough to overflow m_pageBuffer is handled
    // safely by making room first rather than corrupting adjacent bytes.
    if (m_bufferedLength + length > PAGE_BUFFER_CAPACITY) {
        flushBuffer();
        m_bufferedAddress = address;
    }

    // No page-boundary chunking or flushing here anymore - just accumulate. writeBufferedIfReady()
    // (every tick, via run()) and flushBuffer() are what turn this into paced, page-boundary-respecting
    // pageProgram() calls; see PAGE_BUFFER_CAPACITY's comment for why this can't overflow in practice.
    memcpy(m_pageBuffer + m_bufferedLength, buffer, length);
    m_bufferedLength += length;
}

void FlashMemoryCommon::programAndShiftBuffer(uint32_t pageBytes) const {
    // Waits for the *previous* flush to finish, not this one - same non-blocking contract as before.
    if (!ready()) {
        waitForReady(1000);
    }
    pageProgram(m_bufferedAddress, m_pageBuffer, pageBytes);
    m_bufferedAddress += pageBytes;
    m_bufferedLength -= pageBytes;
    if (m_bufferedLength > 0) {
        memmove(m_pageBuffer, m_pageBuffer + pageBytes, m_bufferedLength);
    }
}

void FlashMemoryCommon::writeBufferedIfReady() const {
    // Skips (never waits) if the chip's still busy from a previous program - see this method's header
    // comment for why that's essentially never the case in practice, and run() just tries again next tick either way.
    if (m_bufferedLength == 0 || !ready()) return;
    const uint32_t pageEnd = (m_bufferedAddress - (m_bufferedAddress % getPageSize())) + getPageSize();
    programAndShiftBuffer(min(m_bufferedLength, pageEnd - m_bufferedAddress));
}

void FlashMemoryCommon::flushBuffer() const {
    // Loops (unlike writeBufferedIfReady()) since a full, immediate flush may need several
    // page-programs if a multi-page backlog has built up - used by write()'s non-contiguous-address
    // case (shouldn't happen for BasicLogger), rare enough that the blocking-wait risk this
    // reintroduces is acceptable there.
    while (m_bufferedLength > 0) {
        const uint32_t pageEnd = (m_bufferedAddress - (m_bufferedAddress % getPageSize())) + getPageSize();
        programAndShiftBuffer(min(m_bufferedLength, pageEnd - m_bufferedAddress));
    }
}

void FlashMemoryCommon::run() {
    writeBufferedIfReady();
}


void FlashMemoryCommon::pageProgram(uint32_t address, const uint8_t* buffer, uint32_t length) const {
    uint8_t pageProgramHeader[5] = {
            PAGE_PROGRAM_CMD, // Page Program command
            (uint8_t)((address >> 24) & 0xFF), // Address byte 3 (MSB)
            (uint8_t)((address >> 16) & 0xFF), // Address byte 2
            (uint8_t)((address >> 8) & 0xFF), // Address byte 1
            (uint8_t)(address & 0xFF) // Address byte 0 (LSB)
        };

    enableWrite();
    enableSelectPin();
    m_spiBus->transfer(pageProgramHeader, sizeof(pageProgramHeader));
    // DMA-driven (still blocks until done), just skipping the old loop's per-byte CPU polling - same
    // CS lifecycle as before. Not non-blocking: CS must stay asserted the whole transfer, and FRAM shares this bus with no way to know a deferred transfer was still in flight.
    m_spiBus->transfer(buffer, nullptr, length);
    disableSelectPin();
}

void FlashMemoryCommon::read(uint32_t address, uint8_t* buffer, uint32_t length) const {
    // Wait for any in-flight write to finish first - this chip can't service a normal read while a
    // page program is still in progress (write() itself never waits for its own completion).
    if (!ready()) {
        waitForReady(1000);
    }

    uint8_t readCommandHeader[5] = {
            READ_4BYTE_CMD, // 4-byte read command
            (uint8_t)((address >> 24) & 0xFF), // Most significant byte
            (uint8_t)((address >> 16) & 0xFF), // Next byte
            (uint8_t)((address >> 8) & 0xFF), // Next byte
            (uint8_t)(address & 0xFF) // Least significant byte
        };

    enableSelectPin();
    m_spiBus->transfer(readCommandHeader, sizeof(readCommandHeader));
    m_spiBus->transfer(nullptr, buffer, length); // DMA-driven, same per-byte-overhead savings as pageProgram()
    disableSelectPin();
}

uint8_t FlashMemoryCommon::read(uint32_t address) const {
    uint8_t byte;
    read(address, &byte, 1);
    return byte;
}

void FlashMemoryCommon::write(uint32_t address, uint8_t byte) const {
    write(address, &byte, 1);
}

void FlashMemoryCommon::eraseAll(bool waitForCompletion) const {
    // Discard anything buffered but not yet sent to the chip - it's about to be wiped, and flushing
    // it after this erase would silently restore stale pre-erase data.
    m_bufferedLength = 0;
    // Wait for any pending write first - write() no longer waits for its own completion, and most
    // SPI NOR flash silently ignores a new opcode (like this erase) while WIP is still set.
    if (!ready()) {
        waitForReady(1000);
    }

    enableWrite();
    enableSelectPin();
    m_spiBus->transfer(CHIP_ERASE_CMD);
    disableSelectPin();
    if (waitForCompletion) {
        waitForWriteCompletion(m_deviceData.eraseAllTime);
    }
}

void FlashMemoryCommon::eraseSector(uint32_t sectorNumber, bool waitForCompletion) const {
    // Same reasoning as eraseAll() - see there. Unconditional rather than checked against the
    // sector's range since no current caller uses eraseSector(), so simplicity wins.
    m_bufferedLength = 0;
    if (!ready()) {
        waitForReady(1000);
    }

    uint32_t address = sectorNumber * m_deviceData.sectorSize;
    uint8_t pageProgramHeader[5] = {
            SECTOR_ERASE_CMD, // Page Program command
            (uint8_t)((address >> 24) & 0xFF), // Address byte 3 (MSB)
            (uint8_t)((address >> 16) & 0xFF), // Address byte 2
            (uint8_t)((address >> 8) & 0xFF), // Address byte 1
            (uint8_t)(address & 0xFF) // Address byte 0 (LSB)
        };

    enableWrite();
    enableSelectPin();
    m_spiBus->transfer(pageProgramHeader, sizeof(pageProgramHeader));
    disableSelectPin();
    if (waitForCompletion) {
        waitForWriteCompletion();
    }
}


bool FlashMemoryCommon::waitForWriteCompletion(uint32_t timeout) const {
    uint32_t end = millis() + timeout;
    while (isWriteInProgress()) {
        if (m_watchdog) m_watchdog->pet(); // eraseAll can block for minutes here - see eraseAllTime
        if (millis() >= end) {
            return false;
        }
    }
    return true;
}

bool FlashMemoryCommon::isWriteInProgress() const {
    return (readStatusRegister() & STATUS_WRITE_IN_PROGRESS_BIT) == 1;
}

uint8_t FlashMemoryCommon::readStatusRegister() const {
    uint8_t status;
    enableSelectPin();
    m_spiBus->transfer(STATUS_CMD); // Read Status Register (RDSR) command
    status = m_spiBus->transfer(CLOCK_SPI_DATA); // Dummy byte to clock out the status register
    disableSelectPin();
    return status;
}

void FlashMemoryCommon::enableWrite() const {
    enableSelectPin();
    m_spiBus->transfer(ENABLE_WRITE_CMD);
    disableSelectPin();
}

void FlashMemoryCommon::disableWrite() const {
    enableSelectPin();
    m_spiBus->transfer(DISABLE_WRITE_CMD);
    disableSelectPin();
}

void FlashMemoryCommon::enableSelectPin() const {
    // Re-asserted every transaction, not just at setup(): FRAM shares this SPI bus and sets its own
    // SPISettings on every access, so without this, FRAM's last settings would silently stick.
    m_spiBus->beginTransaction(SPISettings(FLASH_SPI_CLOCK_HZ, MSBFIRST, SPI_MODE0));
    digitalWrite(m_chipSelectPin, LOW);
}

void FlashMemoryCommon::disableSelectPin() const {
    digitalWrite(m_chipSelectPin, HIGH);
    m_spiBus->endTransaction();
}

bool FlashMemoryCommon::waitForReady(uint32_t timeout) const {
    return waitForWriteCompletion(timeout);
}

uint32_t FlashMemoryCommon::getMemorySizeBytes() const {
    return getMemorySize();
}
