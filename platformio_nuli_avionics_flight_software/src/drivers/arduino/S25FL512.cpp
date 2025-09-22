#include "S25FL512.h"
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
#define ERASE_ALL_TIME (1000 * 60 * 5)
#define SECTOR_SIZE (262144)        // Address of end of sector 1 from datasheet: 0003FFFF
#define MEMORY_SIZE (67108864)
#define PAGE_SIZE (512)

/**
 * @todo
 * Figure out where we wait for completion
 * Figure out when we call write disable
 * Standardize API
 * Handle cross page boundary's
 */

S25FL512::S25FL512(const uint8_t chipSelectPin) : m_chipSelectPin(chipSelectPin) {}

void S25FL512::setSpiClass(SPIClass* spiClass) {
    m_spiBus = spiClass;
}

void S25FL512::setup() {
    m_spiBus->begin();
    pinMode(m_chipSelectPin, OUTPUT);
    disableSelectPin();
}


uint32_t S25FL512::getMemorySize() {
    return MEMORY_SIZE;
}

uint32_t S25FL512::getSectorSize() {
    return SECTOR_SIZE;
}

uint32_t S25FL512::getSectorNum() {
    return MEMORY_SIZE / SECTOR_SIZE;
}

uint32_t S25FL512::getPageSize() {
    return PAGE_SIZE;
}


bool S25FL512::ready() const {
    return isWriteInProgress();
}


void S25FL512::write(uint32_t address, const uint8_t* buffer, uint32_t length, bool waitForCompletion) const {
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


void S25FL512::pageProgram(uint32_t address, const uint8_t* buffer, uint32_t length) const {
    uint8_t pageProgramHeader[5] = {
            PAGE_PROGRAM_CMD,                        // Page Program command
            (uint8_t) ((address >> 24) & 0xFF),      // Address byte 3 (MSB)
            (uint8_t) ((address >> 16) & 0xFF),      // Address byte 2
            (uint8_t) ((address >> 8) & 0xFF),       // Address byte 1
            (uint8_t) (address & 0xFF)               // Address byte 0 (LSB)
    };

    enableWrite();
    enableSelectPin();
    m_spiBus->transfer(pageProgramHeader, sizeof(pageProgramHeader));
    for (size_t i = 0; i < length; i++) {
        m_spiBus->transfer(buffer[i]);
    }
    disableSelectPin();
}

void S25FL512::read(uint32_t address, uint8_t* buffer, uint32_t length) const {
    uint8_t readCommandHeader[5] = {
            READ_4BYTE_CMD,                     // 4-byte read command
            (uint8_t) ((address >> 24) & 0xFF),  // Most significant byte
            (uint8_t) ((address >> 16) & 0xFF),  // Next byte
            (uint8_t) ((address >> 8) & 0xFF),   // Next byte
            (uint8_t) (address & 0xFF)           // Least significant byte
    };

    enableSelectPin();
    m_spiBus->transfer(readCommandHeader, sizeof(readCommandHeader));
    for (size_t i = 0; i < length; i++) {
        buffer[i] = m_spiBus->transfer(CLOCK_SPI_DATA);  // Read data into the buffer
    }
    disableSelectPin();
}

uint8_t S25FL512::read(uint32_t address) const {
    uint8_t byte;
    read(address, &byte, 1);
    return byte;
}

void S25FL512::write(uint32_t address, uint8_t byte) const {
    write(address, &byte, 1, true);
}

void S25FL512::eraseAll(bool waitForCompletion) const {
    enableWrite();
    enableSelectPin();
    m_spiBus->transfer(CHIP_ERASE_CMD);
    disableSelectPin();
    if (waitForCompletion) {
        waitForWriteCompletion(ERASE_ALL_TIME);
    }
}

void S25FL512::eraseSector(uint32_t sectorNumber, bool waitForCompletion) const {
    uint32_t address = sectorNumber * SECTOR_SIZE;
    uint8_t pageProgramHeader[5] = {
            SECTOR_ERASE_CMD,                        // Page Program command
            (uint8_t) ((address >> 24) & 0xFF),      // Address byte 3 (MSB)
            (uint8_t) ((address >> 16) & 0xFF),      // Address byte 2
            (uint8_t) ((address >> 8) & 0xFF),       // Address byte 1
            (uint8_t) (address & 0xFF)               // Address byte 0 (LSB)
    };

    enableWrite();
    enableSelectPin();
    m_spiBus->transfer(pageProgramHeader, sizeof(pageProgramHeader));
    disableSelectPin();
    if (waitForCompletion) {
        waitForWriteCompletion();
    }
}


bool S25FL512::waitForWriteCompletion(uint32_t timeout) const {
    uint32_t end = millis() + timeout;
    while (isWriteInProgress()) {
        if (millis() >= end) {
            return false;
        }
    }
    return true;
}

bool S25FL512::isWriteInProgress() const {
    return (readStatusRegister() & STATUS_WRITE_IN_PROGRESS_BIT) == 1;
}

uint8_t S25FL512::readStatusRegister() const {
    uint8_t status;
    enableSelectPin();
    m_spiBus->transfer(STATUS_CMD);  // Read Status Register (RDSR) command
    status = m_spiBus->transfer(CLOCK_SPI_DATA);  // Dummy byte to clock out the status register
    disableSelectPin();
    return status;
}

void S25FL512::enableWrite() const {
    enableSelectPin();
    m_spiBus->transfer(ENABLE_WRITE_CMD);
    disableSelectPin();
}

void S25FL512::disableWrite() const {
    enableSelectPin();
    m_spiBus->transfer(DISABLE_WRITE_CMD);
    disableSelectPin();
}

void S25FL512::enableSelectPin() const {
    digitalWrite(m_chipSelectPin, LOW);
}

void S25FL512::disableSelectPin() const {
    digitalWrite(m_chipSelectPin, HIGH);
}

bool S25FL512::waitForReady(uint32_t timeout) const {
    return waitForWriteCompletion(timeout);
}

uint32_t S25FL512::getMemorySizeBytes() const {
    return getMemorySize();
}















