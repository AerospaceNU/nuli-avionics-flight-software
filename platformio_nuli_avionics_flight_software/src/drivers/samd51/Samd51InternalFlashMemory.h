#ifndef SAMD51INTERNALFLASHMEMORY_H
#define SAMD51INTERNALFLASHMEMORY_H

#include <Arduino.h>
#include <cstring>
#include "core/generic_hardware/FramMemory.h"
#include "core/generic_hardware/WatchdogTimer.h"

/**
 * @class Samd51InternalFlashMemory
 * @brief FRAM stand-in for boards without one: a RAM copy saved to the last 8 KB block of the SAMD51's own flash.
 * @details Writes go to RAM; run() saves them once per tick, and only if the contents changed.
 * Survives power cycles and UF2 updates (the bootloader only writes the image's pages).
 * Each save erases the block, so this suits rarely changed data like configuration.
 * @tparam N Bytes stored; at most one block
 */
template <unsigned N>
class Samd51InternalFlashMemory : public FramMemory {
public:
    void setup(DebugStream* debugStream, WatchdogTimer* watchdog) override {
        m_watchdog = watchdog;
        memcpy(m_buffer, reinterpret_cast<const void*>(STORAGE_ADDRESS), N);
    }

    void run() override {
        if (!m_dirty) return;
        m_dirty = false;
        if (memcmp(m_buffer, reinterpret_cast<const void*>(STORAGE_ADDRESS), N) == 0) return;
        save();
    }

    void write(uint32_t address, const uint8_t* buffer, uint32_t length) override {
        if (address + length > N) return;
        memcpy(m_buffer + address, buffer, length);
        m_dirty = true;
    }

    void read(uint32_t address, uint8_t* buffer, uint32_t length) override {
        if (address + length > N) return;
        memcpy(buffer, m_buffer + address, length);
    }

private:
    static constexpr uint32_t BLOCK_SIZE = 8192;
    static constexpr uint32_t PAGE_SIZE = FLASH_PAGE_SIZE;
    static constexpr uint32_t STORAGE_ADDRESS = FLASH_SIZE - BLOCK_SIZE;
    static constexpr uint32_t NUM_PAGES = (N + PAGE_SIZE - 1) / PAGE_SIZE;
    static_assert(N <= BLOCK_SIZE, "Samd51InternalFlashMemory holds at most one block");

    static void waitReady() {
        while (NVMCTRL->STATUS.bit.READY == 0) {}
    }

    static void command(uint32_t cmd) {
        NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | cmd;
        waitReady();
    }

    void save() {
        if (m_watchdog) m_watchdog->pet();
        const uint16_t savedCtrlA = NVMCTRL->CTRLA.reg;
        // Manual write mode, caches off so later reads see the new contents
        NVMCTRL->CTRLA.reg = (savedCtrlA & ~NVMCTRL_CTRLA_WMODE_Msk) | NVMCTRL_CTRLA_WMODE_MAN | NVMCTRL_CTRLA_CACHEDIS0 | NVMCTRL_CTRLA_CACHEDIS1;
        waitReady();

        NVMCTRL->ADDR.reg = STORAGE_ADDRESS;
        command(NVMCTRL_CTRLB_CMD_EB);

        for (uint32_t page = 0; page < NUM_PAGES; page++) {
            if (m_watchdog) m_watchdog->pet();
            command(NVMCTRL_CTRLB_CMD_PBC);
            // The page buffer only takes 32-bit writes; the unused tail stays erased (0xFF)
            volatile uint32_t* destination = reinterpret_cast<volatile uint32_t*>(STORAGE_ADDRESS + page * PAGE_SIZE);
            for (uint32_t word = 0; word < PAGE_SIZE / 4; word++) {
                const uint32_t offset = page * PAGE_SIZE + word * 4;
                uint32_t value = 0xFFFFFFFF;
                if (offset < N) memcpy(&value, m_buffer + offset, (N - offset) < 4 ? (N - offset) : 4);
                destination[word] = value;
            }
            NVMCTRL->ADDR.reg = STORAGE_ADDRESS + page * PAGE_SIZE;
            command(NVMCTRL_CTRLB_CMD_WP);
        }

        NVMCTRL->CTRLA.reg = savedCtrlA;
        waitReady();
        if (m_watchdog) m_watchdog->pet();
    }

    WatchdogTimer* m_watchdog = nullptr;
    alignas(4) uint8_t m_buffer[N]{};
    bool m_dirty = false;
};

#endif //SAMD51INTERNALFLASHMEMORY_H
