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

/**
 * @todo
 * Figure out where we wait for completion
 * Figure out when we call write disable
 * Standardize API
 */

FlashMemoryCommon::FlashMemoryCommon(const uint8_t chipSelectPin,
                                     const FlashMemoryData_s& deviceData)
    : m_chipSelectPin(chipSelectPin), m_deviceData(deviceData) {}

FlashMemoryCommon::~FlashMemoryCommon() = default;

void FlashMemoryCommon::setSpiClass(SPIClass* spiClass) {
    m_spiBus = spiClass;
}

void FlashMemoryCommon::setup(DebugStream* debugStream) {
    // @todo check a register to make sure the device is there
    m_spiBus->begin();
    pinMode(m_chipSelectPin, OUTPUT);
    disableSelectPin();
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
    return isWriteInProgress();
}


void FlashMemoryCommon::write(uint32_t address, const uint8_t* buffer, uint32_t length, bool waitForCompletion) const {
    uint32_t currentPageEnd;
    uint32_t currentPageBytes;
    uint32_t bufferOffset = 0;
    while (length > 0) {
        // We need to wait for the previous write to finish, regardless of waitForCompletion
        if (!ready()) {
            waitForReady(1000);
        }
        // Get the end of the current page
        currentPageEnd = (address - (address % getPageSize())) + getPageSize();
        // Number of bytes we can write to this page
        currentPageBytes = min(length, currentPageEnd - address);
        pageProgram(address, (buffer + bufferOffset), currentPageBytes);
        if (waitForCompletion) {
            waitForReady(1000);
        }
        // Track that we wrote a certain number of bytes
        length -= currentPageBytes;
        address += currentPageBytes;
        bufferOffset += currentPageBytes;
    }
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
    for (size_t i = 0; i < length; i++) {
        m_spiBus->transfer(buffer[i]);
    }
    disableSelectPin();
}

void FlashMemoryCommon::read(uint32_t address, uint8_t* buffer, uint32_t length) const {
    uint8_t readCommandHeader[5] = {
            READ_4BYTE_CMD, // 4-byte read command
            (uint8_t)((address >> 24) & 0xFF), // Most significant byte
            (uint8_t)((address >> 16) & 0xFF), // Next byte
            (uint8_t)((address >> 8) & 0xFF), // Next byte
            (uint8_t)(address & 0xFF) // Least significant byte
        };

    enableSelectPin();
    m_spiBus->transfer(readCommandHeader, sizeof(readCommandHeader));
    for (size_t i = 0; i < length; i++) {
        buffer[i] = m_spiBus->transfer(m_deviceData.clockSpiData); // Read data into the buffer
    }
    disableSelectPin();
}

uint8_t FlashMemoryCommon::read(uint32_t address) const {
    uint8_t byte;
    read(address, &byte, 1);
    return byte;
}

void FlashMemoryCommon::write(uint32_t address, uint8_t byte) const {
    write(address, &byte, 1, true);
}

void FlashMemoryCommon::eraseAll(bool waitForCompletion) const {
    enableWrite();
    enableSelectPin();
    m_spiBus->transfer(CHIP_ERASE_CMD);
    disableSelectPin();
    if (waitForCompletion) {
        waitForWriteCompletion(m_deviceData.eraseAllTime);
    }
}

void FlashMemoryCommon::eraseSector(uint32_t sectorNumber, bool waitForCompletion) const {
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
    status = m_spiBus->transfer(m_deviceData.clockSpiData); // Dummy byte to clock out the status register
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
    digitalWrite(m_chipSelectPin, LOW);
}

void FlashMemoryCommon::disableSelectPin() const {
    digitalWrite(m_chipSelectPin, HIGH);
}

bool FlashMemoryCommon::waitForReady(uint32_t timeout) const {
    return waitForWriteCompletion(timeout);
}

uint32_t FlashMemoryCommon::getMemorySizeBytes() const {
    return getMemorySize();
}
