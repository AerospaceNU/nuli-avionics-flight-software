#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPFLASH_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPFLASH_H

#include "Avionics.h"
#include "core/generic_hardware/FlashMemory.h"
#include <vector>

/**
 * @class DesktopFlash
 * @brief NOR flash in RAM: writes can only clear bits, erasing sets them back to 0xFF
 */
class DesktopFlash final : public FlashMemory {
public:
    explicit DesktopFlash(const uint32_t sizeBytes, const uint32_t sectorSizeBytes = 4096) :
        m_memory(sizeBytes, 0xFF), m_sectorSizeBytes(sectorSizeBytes) {}

    bool ready() const override { return true; }

    bool waitForReady(uint32_t timeout) const override { return true; }

    void write(const uint32_t address, const uint8_t* buffer, const uint32_t length) const override {
        for (uint32_t i = 0; i < length && address + i < m_memory.size(); i++) m_memory[address + i] &= buffer[i];
    }

    void read(const uint32_t address, uint8_t* buffer, const uint32_t length) const override {
        for (uint32_t i = 0; i < length; i++) buffer[i] = read(address + i);
    }

    uint8_t read(const uint32_t address) const override {
        return address < m_memory.size() ? m_memory[address] : 0xFF;
    }

    void write(const uint32_t address, const uint8_t byte) const override {
        write(address, &byte, 1);
    }

    void eraseAll(bool waitForCompletion) const override {
        std::fill(m_memory.begin(), m_memory.end(), 0xFF);
    }

    void eraseSector(const uint32_t sectorNumber, bool waitForCompletion) const override {
        const uint32_t start = sectorNumber * m_sectorSizeBytes;
        for (uint32_t i = start; i < start + m_sectorSizeBytes && i < m_memory.size(); i++) m_memory[i] = 0xFF;
    }

    uint32_t getMemorySizeBytes() const override {
        return (uint32_t)m_memory.size();
    }

private:
    mutable std::vector<uint8_t> m_memory;
    const uint32_t m_sectorSizeBytes;
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPFLASH_H
