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
        uint16_t crc = 0; // CRC16 over id+data - lets offload() callers detect a torn/corrupted entry
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
               Configuration* configuration = nullptr) {
        m_hardware = hardware;
        m_debug = m_hardware->getDebugStream();
        m_flash = m_hardware->getFlashMemory(flashID);
        m_logWriteIndex = 0;
        m_headerStr = header;
        m_printFunction = printFunction;
        m_configuration = configuration;
        m_watchdog = &m_hardware->getWatchdogTimer(); // single instance, owned by HardwareAbstraction - not injected separately
        m_loopTime = hardware->getTargetLoopTimeMs();

        // Setup CLI interface
        m_offloadBinaryFlag.setDependency(&m_offloadBinaryFlag);
        parser->addFlagGroup(m_eraseGroup);
        parser->addFlagGroup(m_offloadGroup);
        parser->addFlagGroup(m_logGroup);
        parser->addFlagGroup(m_streamGroup);

        m_numEntries = m_flash->getMemorySizeBytes() / sizeof(InternalStruct_s);

        uint32_t left = 0;
        uint32_t right = m_numEntries; // exclusive upper bound
        uint32_t firstEmpty = m_numEntries; // default if nothing is LOG_EMPTY

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

    // Snapshots the whole configuration (every registered field, whatever they are - no per-board
    // struct to keep in sync) to flash, versioned and key-CRC'd so an offload can tell whether it's
    // still safe to decode later. Called once at boot and once at ASCENT (launch), since config can
    // legitimately change between the two (CLI edits on the pad) and a simulator replaying this
    // flight later needs to know which one actually applied.
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

    // Does not verify crc itself - callers that need to know an entry wasn't torn/corrupted should
    // compare computeCrc(m_dataStruct) against m_dataStruct.crc afterward (see offloadCallback()).
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

    void offloadCallback() {
        if (m_offloadBinaryFlag.isSet()) { binaryOffloadCallback(); return; }
        uint32_t failCount = 0;
        m_debug->message("Starting Offload");
        m_debug->data(m_headerStr);

        // Bumped from 512: a reconstructed CONFIG line now includes every registered field
        // (previously a hand-picked subset), including LAUNCH_ANGLE/GYROSCOPE_BIAS printed
        // component-wise - comfortably exceeds the old size on its own.
        constexpr size_t MSG_BUF_SIZE = 1024;
        char msgBuf[MSG_BUF_SIZE];
        size_t msgBufLen = 0;

        // Header {version, keyCrc} + raw config bytes, exactly as logConfig() writes it.
        constexpr size_t CONFIG_BUF_SIZE = 8 + MAX_CONFIGURATION_LENGTH;
        alignas(std::max_align_t) uint8_t configBuf[CONFIG_BUF_SIZE];
        size_t configBufLen = 0;

        // Bounded by m_numEntries as a backstop: log() normally leaves LOG_EMPTY space near the
        // end, but that's enforced elsewhere - this stops a corrupted/full log scanning forever.
        for (uint32_t i = 0; i < m_numEntries; i++) {
            m_watchdog->pet(); // a full offload can run for minutes

            uint8_t id;
            const LogDataStruct logData = offload(i, id);

            // Flush each pending multi-entry blob when we hit a entry that doesn't continue it -
            // message and config sequences never truly interleave (ASCENT can't fire mid-offload),
            // but each needs its own accumulator/trigger regardless, or a config continuation would
            // flush the (unrelated) message buffer's leftover content as garbage, and vice versa.
            if (id != LOG_MESSAGE_CONTINUATION && msgBufLen > 0) {
                if (msgBufLen >= MSG_BUF_SIZE) msgBufLen = MSG_BUF_SIZE - 1;
                msgBuf[msgBufLen] = '\0';
                m_debug->data("%s", msgBuf);
                msgBufLen = 0;
            }
            if (id != LOG_CONFIG_CONTINUATION && configBufLen > 0) {
                flushConfigBuffer(configBuf, configBufLen, msgBuf, MSG_BUF_SIZE);
                configBufLen = 0;
            }

            // A corrupt entry (id survived a torn write, payload didn't - see computeCrc()) is
            // skipped exactly like an empty one, so external output never sees a foreign line.
            if (id == LOG_EMPTY || computeCrc(m_dataStruct) != m_dataStruct.crc) {
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
            m_debug->data("%s", msgBuf);
        }
        if (configBufLen > 0) flushConfigBuffer(configBuf, configBufLen, msgBuf, MSG_BUF_SIZE);
        m_debug->message("Ending Offload");
    }

    // Binary offload: dumps raw packed [id][data] flash records (crc is internal, stripped before
    // sending) framed by a versioned preamble. Host validates magic + struct size + header CRC, so a
    // local struct change can't be silently misparsed; writeRaw blocks on USB backpressure so saturation can't drop records.
    void binaryOffloadCallback() {
        m_debug->message("Starting Offload");

        // Header CRC fingerprints the struct layout (header string is kept in sync with LogDataStruct), so renames/reorders are detected too.
        const uint16_t dataSize = sizeof(LogDataStruct);
        const uint16_t headerCrc = crc16(m_headerStr, strlen(m_headerStr));
        const uint8_t preamble[] = {
            'S', 'G', 'B',
            (uint8_t)(dataSize & 0xFF), (uint8_t)(dataSize >> 8),
            (uint8_t)(headerCrc & 0xFF), (uint8_t)(headerCrc >> 8),
        };
        if (!m_debug->writeRaw(preamble, sizeof(preamble))) {
            m_debug->error("Binary offload aborted: host stopped draining (preamble)");
            return;
        }

        uint32_t failCount = 0;
        for (uint32_t i = 0; i < m_numEntries; i++) {
            m_watchdog->pet(); // a full offload can run for minutes

            uint8_t id;
            offload(i, id); // loads m_dataStruct = {id, data, crc}
            if (id == LOG_EMPTY || computeCrc(m_dataStruct) != m_dataStruct.crc) {
                if (++failCount >= 4) break;
                continue;
            }
            failCount = 0;
            // Only [id][data] goes over the wire - crc bytes would silently desync the host's
            // fixed 1+dataSize-per-entry framing, since the preamble never describes them.
            if (!m_debug->writeRaw(m_dataStructStart, sizeof(InternalStruct_s) - sizeof(m_dataStruct.crc))) {
                m_debug->error("Binary offload aborted: host stopped draining (entry %d)", i);
                return;
            }
        }
        const uint8_t terminator = LOG_EMPTY;
        m_debug->writeRaw(&terminator, 1);
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
    static uint16_t computeCrc(const InternalStruct_s& entry) {
        return crc16(&entry, sizeof(InternalStruct_s) - sizeof(entry.crc));
    }

    // Computes and stores crc, writes the entry, and advances the write index - the single path
    // log()/logMessage()/newFlight() all go through so none of them can forget the crc step.
    void writeCurrentEntry() {
        m_dataStruct.crc = computeCrc(m_dataStruct);
        m_flash->write(m_logWriteIndex * sizeof(InternalStruct_s), m_dataStructStart, sizeof(InternalStruct_s));
        m_logWriteIndex++;
    }

    // Reconstructs one logConfig() record (header + raw config bytes) into a "CONFIG\t..." line,
    // written into lineBuf then printed. Always decodes from the STORED bytes, never from
    // m_configuration's live values - the record being offloaded may be from a previous boot,
    // whose config could legitimately differ from today's. Only trusts the live field list/offsets
    // to interpret those bytes when both the stored version AND key CRC match today's live ones;
    // otherwise the layout may have changed since this was logged, and guessing would misread it.
    void flushConfigBuffer(const uint8_t* raw, size_t rawLen, char* lineBuf, size_t lineBufSize) {
        if (!m_configuration || rawLen < 8) return;
        uint32_t storedVersion, storedKeyCrc;
        memcpy(&storedVersion, raw, sizeof(storedVersion));
        memcpy(&storedKeyCrc, raw + sizeof(storedVersion), sizeof(storedKeyCrc));
        if (storedVersion != m_configuration->getConfigVersion() || storedKeyCrc != m_configuration->getConfigKeyCrc()) {
            m_debug->data("CONFIG\t<unable to decode: schema changed since logged, version=%u keyCrc=%u>",
                          (unsigned)storedVersion, (unsigned)storedKeyCrc);
            return;
        }
        const size_t headerSize = sizeof(storedVersion) + sizeof(storedKeyCrc);
        m_configuration->formatBufferAsText(storedVersion, raw + headerSize, (uint32_t)(rawLen - headerSize), lineBuf, lineBufSize);
        m_debug->data("%s", lineBuf);
    }

    // Chunks arbitrary bytes across multiple LOG_<X>/LOG_<X>_CONTINUATION records - shared by
    // logMessage() (text) and logConfig() (a version+keyCrc header followed by raw config bytes).
    // No watchdog pet here (unlike the old per-chunk pet this replaced): write() now just buffers in
    // RAM (see FlashMemoryCommon's PAGE_BUFFER_CAPACITY/writeBufferedIfReady()), so this whole loop -
    // bounded to a ~508-byte config or a short human-written message, never today's arbitrarily large
    // blob - finishes in well under a millisecond regardless of chunk count. The main loop's own pet()
    // every ~10ms tick is already far inside the 100ms watchdog timeout without any help from here.
    // (On SAMD21, WatchdogSAMD::reset() busy-waits on a hardware SYNCBUSY bit before it can clear the
    // timer, so calling it here at all - let alone once per chunk - is a real, avoidable cost: back-to-
    // back pets with no gap measured at ~3.5ms/call, turning a 3-chunk config burst into an ~11ms stall.)
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

    BaseFlag* m_logGroup[3] = {&m_logFlag, &m_startFlag, &m_endFlag};
    BaseFlag* m_eraseGroup[1] = {&m_eraseFlag};
    BaseFlag* m_offloadGroup[2] = {&m_offloadFlag, &m_offloadBinaryFlag};
    BaseFlag* m_streamGroup[3] = {&m_streamFlag, &m_startFlag, &m_endFlag};

    HardwareAbstraction* m_hardware = nullptr;
    DebugStream* m_debug = nullptr;
    FlashMemory* m_flash = nullptr;
    WatchdogTimer* m_watchdog = nullptr; ///< Set in setup() from HardwareAbstraction::getWatchdogTimer() - never null afterward (that getter always returns a real object, defaulting to a no-op)
    uint32_t m_logWriteIndex = 0;
    uint32_t m_numEntries = 0; ///< Total flash capacity in log entries; bounds offload scans

    InternalStruct_s m_dataStruct{};
    uint8_t* m_dataStructStart = (uint8_t*)&m_dataStruct;

    uint32_t m_loopTime = 0;
    uint32_t m_ticksPerLog = 1;
    uint32_t m_currentTick = 0;

    const char* m_headerStr = nullptr;
    void (*m_printFunction)(const LogDataStruct&, DebugStream*) = nullptr;

    Configuration* m_configuration = nullptr;
};

#endif //BASICLOGGER_H
