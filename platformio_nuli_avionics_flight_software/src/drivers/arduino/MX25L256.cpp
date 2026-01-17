#include "MX25L256.h"
#include <Arduino.h>
#include <SPI.h>

#define DISABLE_WRITE_CMD 0x04  // Good
#define ENABLE_WRITE_CMD 0x06   // Good
#define CHIP_ERASE_CMD 0xC7     // Good
#define SECTOR_ERASE_CMD 0xDC   //
#define READ_4BYTE_CMD 0x13     // Good
#define PAGE_PROGRAM_CMD 0x12   // Good

#define STATUS_CMD 0x05         // Good
#define STATUS_WRITE_IN_PROGRESS_BIT 0x01   // Good

#define CLOCK_SPI_DATA 0x00
#define ERASE_ALL_TIME (1000 * 60 * 5)
#define SECTOR_SIZE (262144 / 4)        // Address of end of sector 1 from datasheet: 0003FFFF
#define MEMORY_SIZE (33554432)
#define PAGE_SIZE (256)

/**
 * @todo
 * Figure out where we wait for completion
 * Figure out when we call write disable
 * Standardize API
 */

MX25L256::MX25L256(const uint8_t chipSelectPin) : m_chipSelectPin(chipSelectPin) {}

void MX25L256::setSpiClass(SPIClass* spiClass) {
    m_spiBus = spiClass;
}

void MX25L256::setup(DebugStream* debugStream) {
    // @todo check a register to make sure the device is there
    m_spiBus->begin();
    pinMode(m_chipSelectPin, OUTPUT);
    disableSelectPin();
    debugStream->message("MX25L256 initialized");
}


uint32_t MX25L256::getMemorySize() {
    return MEMORY_SIZE;
}

uint32_t MX25L256::getSectorSize() {
    return SECTOR_SIZE;
}

uint32_t MX25L256::getSectorNum() {
    return MEMORY_SIZE / SECTOR_SIZE;
}

uint32_t MX25L256::getPageSize() {
    return PAGE_SIZE;
}


bool MX25L256::ready() const {
    return isWriteInProgress();
}


void MX25L256::write(uint32_t address, const uint8_t* buffer, uint32_t length, bool waitForCompletion) const {
    // pageProgram(address, buffer, length);
    // if (waitForCompletion) {
    //     waitForWriteCompletion();
    // }
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
        // Serial.print("Writing ");
        // Serial.print(currentPageBytes);
        // Serial.print(" to address ");
        // Serial.println(address);
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


void MX25L256::pageProgram(uint32_t address, const uint8_t* buffer, uint32_t length) const {
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

void MX25L256::read(uint32_t address, uint8_t* buffer, uint32_t length) const {
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
        buffer[i] = m_spiBus->transfer(CLOCK_SPI_DATA); // Read data into the buffer
    }
    disableSelectPin();
}

uint8_t MX25L256::read(uint32_t address) const {
    uint8_t byte;
    read(address, &byte, 1);
    return byte;
}

void MX25L256::write(uint32_t address, uint8_t byte) const {
    write(address, &byte, 1, true);
}

void MX25L256::eraseAll(bool waitForCompletion) const {
    enableWrite();
    enableSelectPin();
    m_spiBus->transfer(CHIP_ERASE_CMD);
    disableSelectPin();
    if (waitForCompletion) {
        waitForWriteCompletion(ERASE_ALL_TIME);
    }
}

void MX25L256::eraseSector(uint32_t sectorNumber, bool waitForCompletion) const {
    uint32_t address = sectorNumber * SECTOR_SIZE;
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


bool MX25L256::waitForWriteCompletion(uint32_t timeout) const {
    uint32_t end = millis() + timeout;
    while (isWriteInProgress()) {
        if (millis() >= end) {
            return false;
        }
    }
    return true;
}

bool MX25L256::isWriteInProgress() const {
    return (readStatusRegister() & STATUS_WRITE_IN_PROGRESS_BIT) == 1;
}

uint8_t MX25L256::readStatusRegister() const {
    uint8_t status;
    enableSelectPin();
    m_spiBus->transfer(STATUS_CMD); // Read Status Register (RDSR) command
    status = m_spiBus->transfer(CLOCK_SPI_DATA); // Dummy byte to clock out the status register
    disableSelectPin();
    return status;
}

void MX25L256::enableWrite() const {
    enableSelectPin();
    m_spiBus->transfer(ENABLE_WRITE_CMD);
    disableSelectPin();
}

void MX25L256::disableWrite() const {
    enableSelectPin();
    m_spiBus->transfer(DISABLE_WRITE_CMD);
    disableSelectPin();
}

void MX25L256::enableSelectPin() const {
    digitalWrite(m_chipSelectPin, LOW);
}

void MX25L256::disableSelectPin() const {
    digitalWrite(m_chipSelectPin, HIGH);
}

bool MX25L256::waitForReady(uint32_t timeout) const {
    return waitForWriteCompletion(timeout);
}

uint32_t MX25L256::getMemorySizeBytes() const {
    return getMemorySize();
}
