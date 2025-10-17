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
    BasicLogger() : m_logFlag("--log", "Send start", true, 255, [this]() { this->logCallback(); }),
                    m_startFlag("-b", "", false, 255, []() {}),
                    m_endFlag("-e", "", false, 255, []() {}),
                    m_offloadBinaryFlag("-b", "binary", false, 255, []() {}),
                    m_eraseFlag("--erase", "Send start", true, 255, [this]() { this->eraseCallback(); }),
                    m_offloadFlag("--offload", "Send start", true, 255, [this]() { this->offloadCallback(); }),
                    m_streamFlag("--streamLog", "Send start", true, 255, [this]() { this->streamCallback(); }) {}

    void setup(HardwareAbstraction* hardware, Parser* parser, const uint8_t flashID, const char* header, void (*printFunction)(const LogDataStruct&, DebugStream*)) {
        m_hardware = hardware;
        m_debug = m_hardware->getDebugStream();
        m_flash = m_hardware->getFlashMemory(flashID);
        m_logWriteIndex = 0;
        m_headerStr = header;
        m_printFunction = printFunction;
        m_loopTime = hardware->getTargetLoopTimeMs();

        // Setup CLI interface
        m_offloadBinaryFlag.setDependency(&m_offloadBinaryFlag);
        parser->addFlagGroup(m_eraseGroup);
        parser->addFlagGroup(m_offloadGroup);
        parser->addFlagGroup(m_logGroup);
        parser->addFlagGroup(m_streamGroup);

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
                right = mid; // search left half
            } else {
                left = mid + 1; // search right half
            }
        }

        // firstEmpty is now the first index where id == 0xFF
        m_logWriteIndex = firstEmpty;

        m_debug->message("Logging setup, starting at entry: %d, index: %d", getEntryNumber(), (m_logWriteIndex * sizeof(InternalStruct_s)));
        m_debug->message("Maximum log length (s): %d, Remaining log length (s): %d", getMaxLogLengthSeconds(), getRemainingLogLengthSeconds());
        logMessage("Logger setup");
    }

    void log(const LogDataStruct& logDataStruct) {
        m_currentTick++;
        if (m_enableLogging || m_currentTick >= m_ticksPerLog) {
            if (m_currentTick >= m_ticksPerLog) {
                m_currentTick = 0;
            }
            m_dataStruct.id = 0x01;
            m_dataStruct.data = logDataStruct;
            m_flash->write(m_logWriteIndex * sizeof(InternalStruct_s), m_dataStructStart, sizeof(InternalStruct_s), true);
            m_logWriteIndex++;
        }
        if (m_enableStreaming) {
            m_printFunction(logDataStruct, m_debug);
        }
    }

    void logMessage(const char* str) {
        m_dataStruct.id = 0x03;
        memcpy(&m_dataStruct.data, str, min(sizeof(LogDataStruct), strlen(str) + 1));
        ((char*)(&m_dataStruct.data))[sizeof(LogDataStruct) - 1] = '\0'; // Ensure null termination
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

    void offloadCallback() {
        uint32_t failCount = 0;
        m_debug->message("Starting Offload");
        m_debug->data(m_headerStr);
        for (uint32_t i = 0; true; i++) {
            uint8_t id;
            const LogDataStruct logData = offload(i, id);
            if (id == 0xFF) {
                failCount++;
                if (failCount >= 4) {
                    break;
                }
            } else if (id == 0x01) {
                m_printFunction(logData, m_debug);
            } else if (id == 0x02) {
                m_debug->data("New flight");
            } else if (id == 0x03) {
                char* str = (char*)&logData;
                str[sizeof(LogDataStruct) - 1] = '\0';
                m_debug->data("%s", str);
            }
        }
        m_debug->message("Ending Offload");
    }

    void eraseCallback() {
        m_debug->message("Erasing all");
        erase();
        m_debug->message("Done");
    }

    void enableStreaming() {
        m_enableStreaming = true;
    }

    void disableStreaming() {
        m_enableStreaming = false;
    }

    void logCallback() {
        m_debug->message("Entries in log: %d", getEntryNumber());
        m_debug->message("Remaining log length: %d seconds", getRemainingLogLengthSeconds());
        if (m_startFlag.isSet() && !m_endFlag.isSet()) {
            m_enableLogging = true;
            m_debug->message("Logging enabled");
        } else if (m_endFlag.isSet() && !m_startFlag.isSet()) {
            m_enableLogging = false;
            m_debug->message("Logging disabled");
        } else {
            m_debug->message(m_enableLogging ? "Logging is enabled" : "Logging is disabled");
        }
    }

    void streamCallback() {
        if (m_startFlag.isSet() && !m_endFlag.isSet()) {
            m_enableStreaming = true;
            m_debug->message("Streaming enabled");
        } else if (m_endFlag.isSet() && !m_startFlag.isSet()) {
            m_enableStreaming = false;
            m_debug->message("Streaming disabled");
        } else {
            m_debug->message(m_enableStreaming ? "Streaming is enabled" : "Streaming is disabled");
        }
    }

    void enableContinuousLogging() {
        m_enableLogging = true;
    }

    void setLogDelay(const uint32_t delay) {
        m_ticksPerLog = delay / m_loopTime;
    }

    void disableContinuousLogging() {
        m_enableLogging = false;
    }

    bool isLoggingEnabled() const {
        return m_enableLogging;
    }

private:
    bool m_enableLogging = false;
    bool m_enableStreaming = false;

    SimpleFlag m_logFlag;
    SimpleFlag m_startFlag;
    SimpleFlag m_endFlag;
    SimpleFlag m_offloadBinaryFlag;
    SimpleFlag m_eraseFlag;
    SimpleFlag m_offloadFlag;
    SimpleFlag m_streamFlag;

    BaseFlag* m_logGroup[3] = {&m_logFlag, &m_startFlag, &m_endFlag};
    BaseFlag* m_eraseGroup[1] = {&m_eraseFlag};
    BaseFlag* m_offloadGroup[2] = {&m_offloadFlag, &m_offloadBinaryFlag};
    BaseFlag* m_streamGroup[3] = {&m_streamFlag, &m_startFlag, &m_endFlag};

    HardwareAbstraction* m_hardware = nullptr;
    DebugStream* m_debug = nullptr;
    FlashMemory* m_flash = nullptr;
    uint32_t m_logWriteIndex = 0;

    InternalStruct_s m_dataStruct{};
    uint8_t* m_dataStructStart = (uint8_t*)&m_dataStruct;

    uint32_t m_loopTime = 0;
    uint32_t m_ticksPerLog = 1;
    uint32_t m_currentTick = 0;

    const char* m_headerStr = nullptr;
    void (*m_printFunction)(const LogDataStruct&, DebugStream*) = nullptr;
};

#endif //BASICLOGGER_H
