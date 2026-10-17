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

// Both chips support 100 MHz+; 12 MHz is Adafruit's tested SAMD21 SPI ceiling
static constexpr uint32_t FLASH_SPI_CLOCK_HZ = 12000000;

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
    return !isWriteInProgress();
}


void FlashMemoryCommon::write(uint32_t address, const uint8_t* buffer, uint32_t length) const {
    // Non-contiguous with what's buffered: flush the old data under its own address first
    if (m_bufferedLength > 0 && address != m_bufferedAddress + m_bufferedLength) {
        flushBuffer();
    }
    if (m_bufferedLength == 0) {
        m_bufferedAddress = address;
    }

    if (m_bufferedLength + length > PAGE_BUFFER_CAPACITY) {
        flushBuffer();
        m_bufferedAddress = address;
    }

    memcpy(m_pageBuffer + m_bufferedLength, buffer, length);
    m_bufferedLength += length;
}

void FlashMemoryCommon::programAndShiftBuffer(uint32_t pageBytes) const {
    // Waits for the previous program to finish, not this one
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
    if (m_bufferedLength == 0 || !ready()) return;
    const uint32_t pageEnd = (m_bufferedAddress - (m_bufferedAddress % getPageSize())) + getPageSize();
    programAndShiftBuffer(min(m_bufferedLength, pageEnd - m_bufferedAddress));
}

void FlashMemoryCommon::flushBuffer() const {
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
            PAGE_PROGRAM_CMD,
            (uint8_t)((address >> 24) & 0xFF),
            (uint8_t)((address >> 16) & 0xFF),
            (uint8_t)((address >> 8) & 0xFF),
            (uint8_t)(address & 0xFF)
        };

    enableWrite();
    enableSelectPin();
    m_spiBus->transfer(pageProgramHeader, sizeof(pageProgramHeader));
    // Byte-by-byte: DMA transfer() never completes on SeriousGoose (SAMD51/SERCOM1), cause unknown
    for (uint32_t i = 0; i < length; i++) m_spiBus->transfer(buffer[i]);
    disableSelectPin();
}

void FlashMemoryCommon::read(uint32_t address, uint8_t* buffer, uint32_t length) const {
    // The chip can't service a read while a page program is in progress
    if (!ready()) {
        waitForReady(1000);
    }

    uint8_t readCommandHeader[5] = {
            READ_4BYTE_CMD,
            (uint8_t)((address >> 24) & 0xFF),
            (uint8_t)((address >> 16) & 0xFF),
            (uint8_t)((address >> 8) & 0xFF),
            (uint8_t)(address & 0xFF)
        };

    enableSelectPin();
    m_spiBus->transfer(readCommandHeader, sizeof(readCommandHeader));
    // Byte-by-byte, not DMA - see pageProgram()
    for (uint32_t i = 0; i < length; i++) buffer[i] = m_spiBus->transfer(CLOCK_SPI_DATA);
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
    // Drop unsent buffered data so a later flush can't restore pre-erase data
    m_bufferedLength = 0;
    // SPI NOR flash ignores new opcodes while WIP is set
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
    // Same as eraseAll(); drops the whole buffer, not just this sector's range
    m_bufferedLength = 0;
    if (!ready()) {
        waitForReady(1000);
    }

    uint32_t address = sectorNumber * m_deviceData.sectorSize;
    uint8_t pageProgramHeader[5] = {
            SECTOR_ERASE_CMD,
            (uint8_t)((address >> 24) & 0xFF),
            (uint8_t)((address >> 16) & 0xFF),
            (uint8_t)((address >> 8) & 0xFF),
            (uint8_t)(address & 0xFF)
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
        if (m_watchdog) m_watchdog->petInLoop();
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
    m_spiBus->transfer(STATUS_CMD);
    status = m_spiBus->transfer(CLOCK_SPI_DATA);
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
    // Per transaction: FRAM shares this bus with its own SPISettings
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
