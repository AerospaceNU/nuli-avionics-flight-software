#ifndef BASICLOGGER_H
#define BASICLOGGER_H

#include "Avionics.h"
#include "HardwareAbstraction.h"
#include "configuration/Configuration.h"
#include "cli/Parser.h"

enum LogEntryID : uint8_t {
    LOG_EMPTY = 0xFF,
    LOG_DATA = 0x01,
    LOG_NEW_FLIGHT = 0x02,
    LOG_MESSAGE = 0x03,
    LOG_MESSAGE_CONTINUATION = 0x04,
};

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

    void setup(HardwareAbstraction* hardware, Parser* parser, const uint8_t flashID, const char* header,
               void (*printFunction)(const LogDataStruct&, DebugStream*),
               Configuration* configuration = nullptr,
               void (*printConfigFunction)(Configuration*, char*, size_t) = nullptr) {
        m_hardware = hardware;
        m_debug = m_hardware->getDebugStream();
        m_flash = m_hardware->getFlashMemory(flashID);
        m_logWriteIndex = 0;
        m_headerStr = header;
        m_printFunction = printFunction;
        m_configuration = configuration;
        m_printConfigFunction = printConfigFunction;
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
        uint32_t firstEmpty = numEntries; // default if nothing is LOG_EMPTY

        while (left < right) {
            uint32_t mid = left + (right - left) / 2;
            uint8_t id;
            offload(mid, id);

            if (id == LOG_EMPTY) {
                firstEmpty = mid; // possible candidate
                right = mid; // search left half
            } else {
                left = mid + 1; // search right half
            }
        }

        // firstEmpty is now the first index where id == LOG_EMPTY
        m_logWriteIndex = firstEmpty;

        m_debug->message("Logging setup, starting at entry: %d, index: %d", getEntryNumber(), (m_logWriteIndex * sizeof(InternalStruct_s)));
        m_debug->message("Maximum log length (s): %d, Remaining log length (s): %d", getMaxLogLengthSeconds(), getRemainingLogLengthSeconds());
        logMessage("Logger setup");
        if (m_printConfigFunction && m_configuration) {
            char configBuf[512];
            configBuf[0] = '\0';
            m_printConfigFunction(m_configuration, configBuf, sizeof(configBuf));
            if (configBuf[0] != '\0') logMessage(configBuf);
        }
    }

    void log(const LogDataStruct& logDataStruct) {
        m_currentTick++;
        if (m_enableLogging || m_currentTick >= m_ticksPerLog) {
            if (m_currentTick >= m_ticksPerLog) {
                m_currentTick = 0;
            }
            if (getRemainingLogLengthSeconds() > 5) {
                m_dataStruct.id = LOG_DATA;
                m_dataStruct.data = logDataStruct;
                m_flash->write(m_logWriteIndex * sizeof(InternalStruct_s), m_dataStructStart, sizeof(InternalStruct_s), true);
                m_logWriteIndex++;
            }
        }
        if (m_enableStreaming) {
            m_printFunction(logDataStruct, m_debug);
        }
    }

    void logMessage(const char* str) {
        if (getRemainingLogLengthSeconds() <= 5) return;
        const size_t dataSize = sizeof(LogDataStruct);
        const size_t totalLen = strlen(str) + 1; // include null terminator
        size_t pos = 0;
        bool first = true;
        while (pos < totalLen) {
            m_dataStruct.id = first ? LOG_MESSAGE : LOG_MESSAGE_CONTINUATION;
            first = false;
            const size_t remaining = totalLen - pos;
            const size_t chunk = remaining < dataSize ? remaining : dataSize;
            memcpy(&m_dataStruct.data, str + pos, chunk);
            if (chunk < dataSize) {
                memset(((uint8_t*)&m_dataStruct.data) + chunk, 0, dataSize - chunk);
            }
            m_flash->write(m_logWriteIndex * sizeof(InternalStruct_s), m_dataStructStart, sizeof(InternalStruct_s), true);
            m_logWriteIndex++;
            pos += chunk;
        }
    }

    void newFlight() {
        if (getRemainingLogLengthSeconds() > 5) {
            m_dataStruct.id = LOG_NEW_FLIGHT;
            m_flash->write(m_logWriteIndex * sizeof(InternalStruct_s), m_dataStructStart, sizeof(InternalStruct_s), true);
            m_logWriteIndex++;
        }
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

        constexpr size_t MSG_BUF_SIZE = 512;
        char msgBuf[MSG_BUF_SIZE];
        size_t msgBufLen = 0;

        for (uint32_t i = 0; true; i++) {
            uint8_t id;
            const LogDataStruct logData = offload(i, id);

            // Flush pending multi-entry message when we hit a non-continuation entry
            if (id != LOG_MESSAGE_CONTINUATION && msgBufLen > 0) {
                if (msgBufLen >= MSG_BUF_SIZE) msgBufLen = MSG_BUF_SIZE - 1;
                msgBuf[msgBufLen] = '\0';
                m_debug->data("%s", msgBuf);
                msgBufLen = 0;
            }

            if (id == LOG_EMPTY) {
                failCount++;
                if (failCount >= 4) break;
            } else if (id == LOG_DATA) {
                m_printFunction(logData, m_debug);
            } else if (id == LOG_NEW_FLIGHT) {
                m_debug->data("New flight");
            } else if (id == LOG_MESSAGE || id == LOG_MESSAGE_CONTINUATION) {
                size_t copyLen = sizeof(LogDataStruct);
                if (msgBufLen + copyLen >= MSG_BUF_SIZE) {
                    copyLen = (msgBufLen >= MSG_BUF_SIZE - 1) ? 0 : (MSG_BUF_SIZE - 1 - msgBufLen);
                }
                memcpy(msgBuf + msgBufLen, &logData, copyLen);
                msgBufLen += copyLen;
            }
        }
        if (msgBufLen > 0) {
            if (msgBufLen >= MSG_BUF_SIZE) msgBufLen = MSG_BUF_SIZE - 1;
            msgBuf[msgBufLen] = '\0';
            m_debug->data("%s", msgBuf);
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

    Configuration* m_configuration = nullptr;
    void (*m_printConfigFunction)(Configuration*, char*, size_t) = nullptr;
};

#endif //BASICLOGGER_H
