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
#define SECTOR_SIZE (256000)
#define MEMORY_SIZE (64000000)
#define PAGE_SIZE (512)

/**
 * @todo
 * Figure out where we wait for completion
 * Figure out when we call write disable
 * Standardize API
 * Handle cross page boundary's
 */

S25FL512::S25FL512(uint8_t chipSelectPin, SPIClass* spiClass) : m_chipSelectPin(chipSelectPin) {
//    pinMode(chipSelectPin, OUTPUT);
    m_spiBus = spiClass;
}


void S25FL512::setup() const {
    pinMode(m_chipSelectPin, OUTPUT);
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
    pageProgram(address, buffer, length);
    if (waitForCompletion) {
        waitForWriteCompletion();
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
    SPI.transfer(pageProgramHeader, sizeof(pageProgramHeader));
    for (size_t i = 0; i < length; i++) {
        SPI.transfer(buffer[i]);
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
    SPI.transfer(readCommandHeader, sizeof(readCommandHeader));
    for (size_t i = 0; i < length; i++) {
        buffer[i] = SPI.transfer(CLOCK_SPI_DATA);  // Read data into the buffer
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
    SPI.transfer(CHIP_ERASE_CMD);
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
    SPI.transfer(pageProgramHeader, sizeof(pageProgramHeader));
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
    SPI.transfer(STATUS_CMD);  // Read Status Register (RDSR) command
    status = SPI.transfer(CLOCK_SPI_DATA);  // Dummy byte to clock out the status register
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















