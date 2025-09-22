#ifndef BASICLOGGER_H
#define BASICLOGGER_H

#include "Avionics.h"
#include "HardwareAbstraction.h"
#include "Configuration.h"
#include "cli/Parser.h"

template <typename LogDataStruct>
class BasicLogger {
    // clang-format off
    struct InternalStruct_s {
        uint8_t id = 0;
        LogDataStruct data;
    } remove_struct_padding;
    // clang-format on
public:
    void setup(HardwareAbstraction* hardware, Parser* parser, const uint8_t flashID, const char *header, void (*printFunction)(const LogDataStruct&)) {
        m_hardware = hardware;
        m_flash = m_hardware->getFlashMemory(flashID);
        m_logWriteIndex = 0;
        m_headerStr = header;
        m_printFunction = printFunction;

        // @todo binary search, also time remaining in log
        const uint32_t numEntries = m_flash->getMemorySizeBytes() / sizeof(InternalStruct_s);

        uint32_t left = 0;
        uint32_t right = numEntries; // exclusive upper bound
        uint32_t firstEmpty = numEntries; // default if nothing is 0xFF

        while (left < right) {
            uint32_t mid = left + (right - left) / 2;
            uint8_t id;
            offload(mid, id);

            if (id == 0xFF) {
                firstEmpty = mid; // possible candidate
                right = mid;      // search left half
            } else {
                left = mid + 1;   // search right half
            }
        }

        // firstEmpty is now the first index where id == 0xFF
        m_logWriteIndex = firstEmpty;

        Serial.print("Starting logging at ");
        Serial.print(getEntryNumber());
        Serial.print(" entry, index: ");
        Serial.println(m_logWriteIndex * sizeof(InternalStruct_s));
        Serial.print("Maximum log length (s): ");
        Serial.println(getMaxLogLengthSeconds());
        Serial.print("Remaining log length (s): ");
        Serial.println(getRemainingLogLengthSeconds());
    }

    void log(const LogDataStruct& logDataStruct) {
        m_dataStruct.id = 0x01;
        m_dataStruct.data = logDataStruct;
        m_flash->write(m_logWriteIndex * sizeof(InternalStruct_s), m_dataStructStart, sizeof(InternalStruct_s), true);
        m_logWriteIndex++;
    }

    void newFlight() {
        m_dataStruct.id = 0x02;
        m_flash->write(m_logWriteIndex * sizeof(InternalStruct_s), m_dataStructStart, sizeof(InternalStruct_s), true);
        m_logWriteIndex++;
    }

    void erase() {
        m_flash->eraseAll(true);
        m_logWriteIndex = 0;
    }

    LogDataStruct offload(const uint32_t index, uint8_t& id) {
        m_flash->read(index * sizeof(InternalStruct_s), m_dataStructStart, sizeof(InternalStruct_s));
        id = m_dataStruct.id;
        return m_dataStruct.data;
    }

    uint32_t getEntryNumber() const {
        return m_logWriteIndex;
    }

    uint32_t getMaxLogLengthSeconds() const {
        // Total entries the flash can hold
        const uint32_t totalEntries = m_flash->getMemorySizeBytes() / sizeof(InternalStruct_s);

        // Convert loop time from ms to seconds
        const float loopTimeSec = m_hardware->getTargetLoopTimeMs() / 1000.0f;

        // Maximum duration = total entries * loop time
        return static_cast<uint32_t>(totalEntries * loopTimeSec);
    }

    uint32_t getRemainingLogLengthSeconds() const {
        // Total entries the flash can hold
        const uint32_t totalEntries = m_flash->getMemorySizeBytes() / sizeof(InternalStruct_s);

        // Remaining entries
        const uint32_t remainingEntries = totalEntries - m_logWriteIndex;

        // Convert loop time from ms to seconds
        const float loopTimeSec = m_hardware->getTargetLoopTimeMs() / 1000.0f;

        // Remaining duration = remaining entries * loop time
        return static_cast<uint32_t>(remainingEntries * loopTimeSec);
    }


private:
    HardwareAbstraction* m_hardware = nullptr;
    FlashMemory* m_flash = nullptr;
    uint32_t m_logWriteIndex = 0;

    InternalStruct_s m_dataStruct{};
    uint8_t* m_dataStructStart = (uint8_t*)&m_dataStruct;

    const char *m_headerStr = nullptr;
    void (*m_printFunction)(const LogDataStruct&) = nullptr;
};

#endif //BASICLOGGER_H
