#ifndef BASICLOGGER_H
#define BASICLOGGER_H

#include "Avionics.h"
#include "HardwareAbstraction.h"
#include "configuration/Configuration.h"
#include "cli/Parser.h"
#include "cli/SimpleFlag.h"
#include "util/CRC.h"

enum LogEntryID : uint8_t {
    LOG_EMPTY = 0xFF,
    LOG_DATA = 0x01,
    LOG_NEW_FLIGHT = 0x02,
    LOG_MESSAGE = 0x03,
    LOG_MESSAGE_CONTINUATION = 0x04,
    LOG_CONFIG = 0x05,
    LOG_CONFIG_CONTINUATION = 0x06,
};

template <typename LogDataStruct>
class BasicLogger {
    // clang-format off
    struct InternalStruct_s {
        uint8_t id = 0;
        LogDataStruct data;
        uint16_t crc = 0; // CRC16 over id+data, detects torn/corrupted entries
    } remove_struct_padding;
    // clang-format on

public:
    BasicLogger() : m_logFlag("--log", "Logging status", true, [this](DebugStream* debugStream) { this->logCallback(debugStream); }),
                    m_startFlag("-b", "Begin", false, [](DebugStream*) {}),
                    m_endFlag("-e", "End", false, [](DebugStream*) {}),
                    m_offloadBinaryFlag("-b", "Binary format", false, [](DebugStream*) {}),
                    m_eraseFlag("--erase", "Erases the flight log", true, [this](DebugStream* debugStream) { this->eraseCallback(debugStream); }),
                    // High-bandwidth only (see BaseFlag): silently ignored over CLI-over-radio
                    m_offloadFlag("--offload", "Downloads the flight log", true, [this](DebugStream* debugStream) { this->offloadCallback(debugStream); }, true),
                    m_streamFlag("--streamLog", "Streaming status", true, [this](DebugStream* debugStream) { this->streamCallback(debugStream); }, true),
                    m_formatFlag("--logFormat", "Prints the log data struct definition", true, [this](DebugStream* debugStream) { this->printFormat(debugStream); }, true) {}

    void setup(HardwareAbstraction* hardware, Parser* parser, const uint8_t flashID, const char* header, const char* format,
               void (*printFunction)(const LogDataStruct&, DebugStream*),
               Configuration* configuration = nullptr) {
        m_hardware = hardware;
        m_debug = m_hardware->getDebugStream();
        m_flash = m_hardware->getFlashMemory(flashID);
        m_logWriteIndex = 0;
        m_headerStr = header;
        m_formatStr = format;
        m_printFunction = printFunction;
        m_configuration = configuration;
        m_watchdog = &m_hardware->getWatchdogTimer();
        m_loopTime = hardware->getTargetLoopTimeMs();

        m_offloadBinaryFlag.setDependency(&m_offloadBinaryFlag);
        parser->addFlagGroup(m_eraseGroup);
        parser->addFlagGroup(m_offloadGroup);
        parser->addFlagGroup(m_logGroup);
        parser->addFlagGroup(m_streamGroup);
        parser->addFlagGroup(m_formatGroup);

        m_numEntries = m_flash->getMemorySizeBytes() / sizeof(InternalStruct_s);

        // Binary search for the first LOG_EMPTY entry
        uint32_t left = 0;
        uint32_t right = m_numEntries; // exclusive
        uint32_t firstEmpty = m_numEntries;

        while (left < right) {
            uint32_t mid = left + (right - left) / 2;
            uint8_t id;
            offload(mid, id);

            if (id == LOG_EMPTY) {
                firstEmpty = mid;
                right = mid;
            } else {
                left = mid + 1;
            }
        }

        m_logWriteIndex = firstEmpty;

        m_debug->message("Logging setup, starting at entry: %d, index: %d", getEntryNumber(), (m_logWriteIndex * sizeof(InternalStruct_s)));
        m_debug->message("Maximum log length (s): %d, Remaining log length (s): %d", getMaxLogLengthSeconds(), getRemainingLogLengthSeconds());
        logMessage("Logger setup");
        if (m_configuration) logConfig(m_configuration);
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
                writeCurrentEntry();
            }
        }
        if (m_enableStreaming) {
            m_printFunction(logDataStruct, m_debug);
        }
    }

    void logMessage(const char* str) {
        if (getRemainingLogLengthSeconds() <= 5) return;
        logChunked(reinterpret_cast<const uint8_t*>(str), strlen(str) + 1, LOG_MESSAGE, LOG_MESSAGE_CONTINUATION);
    }

    // Logs every config field with version + key CRC so offload can tell if it is still decodable.
    // Called at boot and again at ASCENT, since pad CLI edits can change config in between.
    void logConfig(Configuration* configuration) {
        if (!configuration) return;
        if (getRemainingLogLengthSeconds() <= 5) return;
        uint32_t configLen = 0;
        const uint8_t* configBytes = configuration->getConfigDataBuffer(configLen);
        if (!configBytes || configLen == 0) return;

        const uint32_t version = configuration->getConfigVersion();
        const uint32_t keyCrc = configuration->getConfigKeyCrc();
        alignas(std::max_align_t) uint8_t buf[sizeof(version) + sizeof(keyCrc) + MAX_CONFIGURATION_LENGTH];
        memcpy(buf, &version, sizeof(version));
        memcpy(buf + sizeof(version), &keyCrc, sizeof(keyCrc));
        memcpy(buf + sizeof(version) + sizeof(keyCrc), configBytes, configLen);
        logChunked(buf, sizeof(version) + sizeof(keyCrc) + configLen, LOG_CONFIG, LOG_CONFIG_CONTINUATION);
    }

    void newFlight() {
        if (getRemainingLogLengthSeconds() > 5) {
            m_dataStruct.id = LOG_NEW_FLIGHT;
            writeCurrentEntry();
        }
    }

    void erase() {
        m_flash->eraseAll(true);
        m_logWriteIndex = 0;
    }

    // Does not check the crc: compare computeCrc(m_dataStruct) with m_dataStruct.crc afterward
    LogDataStruct offload(const uint32_t index, uint8_t& id) {
        m_flash->read(index * sizeof(InternalStruct_s), m_dataStructStart, sizeof(InternalStruct_s));
        id = m_dataStruct.id;
        return m_dataStruct.data;
    }

    uint32_t getEntryNumber() const {
        return m_logWriteIndex;
    }

    uint32_t getMaxLogLengthSeconds() const {
        const float loopTimeSec = m_hardware->getTargetLoopTimeMs() / 1000.0f;
        return static_cast<uint32_t>(m_numEntries * loopTimeSec);
    }

    uint32_t getRemainingLogLengthSeconds() const {
        const float loopTimeSec = m_hardware->getTargetLoopTimeMs() / 1000.0f;
        return static_cast<uint32_t>((m_numEntries - m_logWriteIndex) * loopTimeSec);
    }

    void offloadCallback(DebugStream* debugStream) {
        if (m_offloadBinaryFlag.isSet()) { binaryOffloadCallback(debugStream); return; }
        uint32_t failCount = 0;
        debugStream->message("Starting Offload");
        printFormat(debugStream);
        debugStream->data(m_headerStr);

        // Also holds a reconstructed CONFIG line (every field), which exceeds 512
        constexpr size_t MSG_BUF_SIZE = 1024;
        char msgBuf[MSG_BUF_SIZE];
        size_t msgBufLen = 0;

        // {version, keyCrc} + raw config bytes, as logConfig() writes it
        constexpr size_t CONFIG_BUF_SIZE = 8 + MAX_CONFIGURATION_LENGTH;
        alignas(std::max_align_t) uint8_t configBuf[CONFIG_BUF_SIZE];
        size_t configBufLen = 0;

        // m_numEntries bound is a backstop for a full/corrupted log with no LOG_EMPTY
        for (uint32_t i = 0; i < m_numEntries; i++) {
            m_watchdog->petInLoop(); // a full offload can run for minutes

            uint8_t id;
            const LogDataStruct logData = offload(i, id);

            // Flush a pending message/config blob once an entry doesn't continue it
            if (id != LOG_MESSAGE_CONTINUATION && msgBufLen > 0) {
                if (msgBufLen >= MSG_BUF_SIZE) msgBufLen = MSG_BUF_SIZE - 1;
                msgBuf[msgBufLen] = '\0';
                debugStream->data("%s", msgBuf);
                msgBufLen = 0;
            }
            if (id != LOG_CONFIG_CONTINUATION && configBufLen > 0) {
                flushConfigBuffer(debugStream, configBuf, configBufLen, msgBuf, MSG_BUF_SIZE);
                configBufLen = 0;
            }

            // Corrupt (torn-write) entries are skipped like empty ones
            if (id == LOG_EMPTY || computeCrc(m_dataStruct) != m_dataStruct.crc) {
                if (id == LOG_EMPTY) failCount++;
                if (failCount >= 4) break;
            } else if (id == LOG_DATA) {
                m_printFunction(logData, debugStream);
            } else if (id == LOG_NEW_FLIGHT) {
                debugStream->data("New flight");
            } else if (id == LOG_MESSAGE || id == LOG_MESSAGE_CONTINUATION) {
                size_t copyLen = sizeof(LogDataStruct);
                if (msgBufLen + copyLen >= MSG_BUF_SIZE) {
                    copyLen = (msgBufLen >= MSG_BUF_SIZE - 1) ? 0 : (MSG_BUF_SIZE - 1 - msgBufLen);
                }
                memcpy(msgBuf + msgBufLen, &logData, copyLen);
                msgBufLen += copyLen;
            } else if (id == LOG_CONFIG || id == LOG_CONFIG_CONTINUATION) {
                size_t copyLen = sizeof(LogDataStruct);
                if (configBufLen + copyLen > CONFIG_BUF_SIZE) {
                    copyLen = (configBufLen >= CONFIG_BUF_SIZE) ? 0 : (CONFIG_BUF_SIZE - configBufLen);
                }
                memcpy(configBuf + configBufLen, &logData, copyLen);
                configBufLen += copyLen;
            }
        }
        if (msgBufLen > 0) {
            if (msgBufLen >= MSG_BUF_SIZE) msgBufLen = MSG_BUF_SIZE - 1;
            msgBuf[msgBufLen] = '\0';
            debugStream->data("%s", msgBuf);
        }
        if (configBufLen > 0) flushConfigBuffer(debugStream, configBuf, configBufLen, msgBuf, MSG_BUF_SIZE);
        debugStream->message("Ending Offload");
    }

    // Sends a preamble (magic, struct size, header CRC) then raw [id][data] records, crc stripped
    void binaryOffloadCallback(DebugStream* debugStream) {
        debugStream->message("Starting Offload");
        printFormat(debugStream);

        // Header string mirrors LogDataStruct, so its CRC also catches field renames/reorders
        const uint16_t dataSize = sizeof(LogDataStruct);
        const uint16_t headerCrc = crc16(m_headerStr, strlen(m_headerStr));
        const uint8_t preamble[] = {
            'S', 'G', 'B',
            (uint8_t)(dataSize & 0xFF), (uint8_t)(dataSize >> 8),
            (uint8_t)(headerCrc & 0xFF), (uint8_t)(headerCrc >> 8),
        };
        if (!debugStream->writeRaw(preamble, sizeof(preamble))) {
            debugStream->error("Binary offload aborted: host stopped draining (preamble)");
            return;
        }

        uint32_t failCount = 0;
        for (uint32_t i = 0; i < m_numEntries; i++) {
            m_watchdog->petInLoop(); // a full offload can run for minutes

            uint8_t id;
            offload(i, id); // loads m_dataStruct = {id, data, crc}
            if (id == LOG_EMPTY) {
                if (++failCount >= 4) break;
                continue;
            }
            if (computeCrc(m_dataStruct) != m_dataStruct.crc) continue;
            // Host framing is 1+dataSize per entry: never send the crc
            if (!debugStream->writeRaw(m_dataStructStart, sizeof(InternalStruct_s) - sizeof(m_dataStruct.crc))) {
                debugStream->error("Binary offload aborted: host stopped draining (entry %d)", i);
                return;
            }
        }
        const uint8_t terminator = LOG_EMPTY;
        debugStream->writeRaw(&terminator, 1);
        debugStream->message("Ending Offload");
    }

    void eraseCallback(DebugStream* debugStream) {
        debugStream->message("Erasing all");
        erase();
        debugStream->message("Done");
    }

    void enableStreaming() {
        m_enableStreaming = true;
    }

    void disableStreaming() {
        m_enableStreaming = false;
    }

    void logCallback(DebugStream* debugStream) {
        debugStream->message("Entries in log: %d", getEntryNumber());
        debugStream->message("Remaining log length: %d seconds", getRemainingLogLengthSeconds());
        if (m_startFlag.isSet() && !m_endFlag.isSet()) {
            m_enableLogging = true;
            debugStream->message("Logging enabled");
        } else if (m_endFlag.isSet() && !m_startFlag.isSet()) {
            m_enableLogging = false;
            debugStream->message("Logging disabled");
        } else {
            debugStream->message(m_enableLogging ? "Logging is enabled" : "Logging is disabled");
        }
    }

    void streamCallback(DebugStream* debugStream) {
        if (m_startFlag.isSet() && !m_endFlag.isSet()) {
            m_enableStreaming = true;
            printFormat(debugStream);
            debugStream->message("Streaming enabled");
        } else if (m_endFlag.isSet() && !m_startFlag.isSet()) {
            m_enableStreaming = false;
            debugStream->message("Streaming disabled");
        } else {
            debugStream->message(m_enableStreaming ? "Streaming is enabled" : "Streaming is disabled");
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
    void printFormat(DebugStream* debugStream) {
        if (!m_formatStr) return;
        static const char prefix[] = "LOG_FORMAT\t";
        debugStream->writeRaw(prefix, sizeof(prefix) - 1);
        debugStream->writeRaw(m_formatStr, strlen(m_formatStr));
        debugStream->writeRaw("\n", 1);
    }

    static uint16_t computeCrc(const InternalStruct_s& entry) {
        return crc16(&entry, sizeof(InternalStruct_s) - sizeof(entry.crc));
    }

    // Every entry write goes through here so the crc is always set
    void writeCurrentEntry() {
        m_dataStruct.crc = computeCrc(m_dataStruct);
        m_flash->write(m_logWriteIndex * sizeof(InternalStruct_s), m_dataStructStart, sizeof(InternalStruct_s));
        m_logWriteIndex++;
    }

    // Prints a logConfig() record as a "CONFIG\t..." line, decoded from the stored bytes using the
    // live layout only when the stored version and key CRC both match it
    void flushConfigBuffer(DebugStream* debugStream, const uint8_t* raw, size_t rawLen, char* lineBuf, size_t lineBufSize) {
        if (!m_configuration || rawLen < 8) return;
        uint32_t storedVersion, storedKeyCrc;
        memcpy(&storedVersion, raw, sizeof(storedVersion));
        memcpy(&storedKeyCrc, raw + sizeof(storedVersion), sizeof(storedKeyCrc));
        if (storedVersion != m_configuration->getConfigVersion() || storedKeyCrc != m_configuration->getConfigKeyCrc()) {
            debugStream->data("CONFIG\t<unable to decode: schema changed since logged, version=%u keyCrc=%u>",
                          (unsigned)storedVersion, (unsigned)storedKeyCrc);
            return;
        }
        const size_t headerSize = sizeof(storedVersion) + sizeof(storedKeyCrc);
        m_configuration->formatBufferAsText(storedVersion, raw + headerSize, (uint32_t)(rawLen - headerSize), lineBuf, lineBufSize);
        debugStream->data("%s", lineBuf);
    }

    // No watchdog pet: flash writes only buffer in RAM, and a SAMD21 pet costs ~3.5ms
    void logChunked(const uint8_t* bytes, size_t totalLen, LogEntryID firstId, LogEntryID continuationId) {
        const size_t dataSize = sizeof(LogDataStruct);
        size_t pos = 0;
        bool first = true;
        while (pos < totalLen) {
            m_dataStruct.id = first ? firstId : continuationId;
            first = false;
            const size_t remaining = totalLen - pos;
            const size_t chunk = remaining < dataSize ? remaining : dataSize;
            memcpy(&m_dataStruct.data, bytes + pos, chunk);
            if (chunk < dataSize) {
                memset(((uint8_t*)&m_dataStruct.data) + chunk, 0, dataSize - chunk);
            }
            writeCurrentEntry();
            pos += chunk;
        }
    }

    bool m_enableLogging = false;
    bool m_enableStreaming = false;

    SimpleFlag m_logFlag;
    SimpleFlag m_startFlag;
    SimpleFlag m_endFlag;
    SimpleFlag m_offloadBinaryFlag;
    SimpleFlag m_eraseFlag;
    SimpleFlag m_offloadFlag;
    SimpleFlag m_streamFlag;
    SimpleFlag m_formatFlag;

    BaseFlag* m_logGroup[3] = {&m_logFlag, &m_startFlag, &m_endFlag};
    BaseFlag* m_eraseGroup[1] = {&m_eraseFlag};
    BaseFlag* m_offloadGroup[2] = {&m_offloadFlag, &m_offloadBinaryFlag};
    BaseFlag* m_streamGroup[3] = {&m_streamFlag, &m_startFlag, &m_endFlag};
    BaseFlag* m_formatGroup[1] = {&m_formatFlag};

    HardwareAbstraction* m_hardware = nullptr;
    DebugStream* m_debug = nullptr;
    FlashMemory* m_flash = nullptr;
    WatchdogTimer* m_watchdog = nullptr; ///< Never null after setup()
    uint32_t m_logWriteIndex = 0;
    uint32_t m_numEntries = 0; ///< Flash capacity in log entries

    InternalStruct_s m_dataStruct{};
    uint8_t* m_dataStructStart = (uint8_t*)&m_dataStruct;

    uint32_t m_loopTime = 0;
    uint32_t m_ticksPerLog = 1;
    uint32_t m_currentTick = 0;

    const char* m_headerStr = nullptr;
    const char* m_formatStr = nullptr;
    void (*m_printFunction)(const LogDataStruct&, DebugStream*) = nullptr;

    Configuration* m_configuration = nullptr;
};

#endif //BASICLOGGER_H
