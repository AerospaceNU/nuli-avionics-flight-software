#ifndef GROUNDSTATIONRELAY_H
#define GROUNDSTATIONRELAY_H

#include "Avionics.h"
#include "cli/Parser.h"
#include "cli/ArgumentFlag.h"
#include "generic_hardware/RadioLink.h"
#include "generic_hardware/GPS.h"
#include "util/Timer.h"
#include <cstring>

// Lets a board relay another board's radio telemetry to USB and report its own GPS fix, instead
// of acting as a flight computer - formerly the standalone SeriousGooseGround.cpp firmware, now
// just a mode a shared board can be switched into (see GROUND_STATION_MODE_c). Assumes the caller
// already owns/configures the actual radio and GPS hardware (same board, same Configuration).
class GroundStationRelay {
public:
    GroundStationRelay() : m_sendFlag("--send", "Queues a radio uplink payload", true, [this](DebugStream* debugStream) { this->sendCallback(debugStream); }) {}

    void setup(Parser* parser, RadioLink* radio, GPS* gps) {
        m_radio = radio;
        m_gps = gps;
        parser->addFlagGroup(m_sendGroup);
        m_gpsReportTimer.startAlarm(0, 0); // Trigger right away
    }

    // Call once per loop iteration in place of the normal flight-computer radio TX/RX handling.
    void tick(const Timestamp_s& timestamp, DebugStream* debugStream) {
        // Periodically report this board's own GPS position over USB - suppressed while a CLI
        // response looks to be actively draining (a chunk seen recently), since GPS reporting
        // runs on its own timer independent of the radio's packet cadence and would otherwise
        // inject a "GPS\t..." line into the middle of a still-unterminated response fragment
        // from a prior tick. Time-based (not a sticky flag cleared on the next telemetry packet)
        // so it self-heals even if the flying board goes silent mid-response.
        const bool receivingCliResponse = m_lastCliResponseRxTimeMs != 0 &&
            (timestamp.runtime_ms - m_lastCliResponseRxTimeMs) < CLI_RESPONSE_QUIET_MS;
        if (!receivingCliResponse && m_gpsReportTimer.isAlarmFinished(timestamp.runtime_ms)) {
            const Coordinates_s coords = m_gps->getCoordinates();
            debugStream->data("GPS\t%.6f\t%.6f\t%.2f\t%lu\t%u\t%u\t%d\t%d",
                coords.latitudeDeg, coords.longitudeDeg, coords.altitudeM,
                m_gps->getUnixTimeS(), m_gps->getHDOP(), m_gps->getVDOP(), m_gps->getFixQuality(), m_gps->getSatellitesTracked());
            m_gpsReportTimer.startAlarm(timestamp.runtime_ms, GPS_REPORT_INTERVAL_MS);
        }

        // Forward any received radio packet (raw, hex-encoded bytes) over USB - decoding is left to
        // the host tool, which knows the flight computer's log struct layout
        if (m_radio->isMessageAvailable()) {
            const RadioLink::RadioMessage message = m_radio->readMessage();

            if (m_lastDownlinkRxTimeMs != 0) {
                const uint32_t rawInterval = timestamp.runtime_ms - m_lastDownlinkRxTimeMs;
                uint32_t impliedCadence = rawInterval;
                if (m_downlinkCadenceMs > 0) {
                    // ~3x the last cadence means 2 packets were dropped, not that cadence tripled - round to nearest multiple so jitter near a boundary doesn't misclassify.
                    uint32_t multiple = (rawInterval + m_downlinkCadenceMs / 2) / m_downlinkCadenceMs;
                    if (multiple == 0) multiple = 1;
                    impliedCadence = rawInterval / multiple;
                }
                // IIR smoothing (3:1) so one packet's jitter doesn't jerk the predicted schedule.
                m_downlinkCadenceMs = (m_downlinkCadenceMs == 0) ? impliedCadence : (m_downlinkCadenceMs * 3 + impliedCadence) / 4;
            }
            m_lastDownlinkRxTimeMs = timestamp.runtime_ms;
            // Send before the USB print below - the safe window is finite (~457ms) and a slow host could eat into it.
            trySendPending();

            if (message.length >= 1 && message.data[0] == RADIO_MSG_CLI_RESPONSE) {
                m_lastCliResponseRxTimeMs = timestamp.runtime_ms;
                // Raw pass-through onto this board's own USB stream - reconstructs the flying
                // board's original CLI text output byte-for-byte (it already contains its own
                // "\n"s), so neither this firmware nor the host tool need to parse/reformat it.
                debugStream->writeRaw(message.data + 1, message.length - 1);
            } else {
                char hexBuffer[sizeof(message.data) * 2 + 1];
                bytesToHex(message.data, message.length, hexBuffer);
                debugStream->data("RADIO_RX\t%d\t%.2f\t%s", message.rssi, message.snr, hexBuffer);
            }
        }

        // Elapsed-time check, not gated on hearing a downlink - keeps firing through lost downlink packets.
        if (m_pendingSendLength > 0 && inSafeUplinkWindow(timestamp.runtime_ms)) {
            trySendPending();
        }
    }

private:
    void sendCallback(DebugStream* debugStream) {
        const char* payload = m_sendFlag.getValueDerived();
        // No null terminator here - the receiving DebugStreamQueue::pushIncoming() appends its
        // own line delimiter, so one embedded here too would double up (an extra '\0' hits
        // readLine() as a phantom empty command next tick). Truncated safely if payload ever
        // exceeded the buffer (CLI input never will).
        uint32_t len = (uint32_t)strlen(payload);
        if (len > sizeof(m_pendingSendBuffer) - 1) len = sizeof(m_pendingSendBuffer) - 1;
        m_pendingSendBuffer[0] = RADIO_MSG_CLI_COMMAND;
        memcpy(m_pendingSendBuffer + 1, payload, len);
        m_pendingSendLength = len + 1;
        debugStream->message("Uplink queued (%lu bytes) - sending at the next predicted safe window", (unsigned long)len);
    }

    // Only fails if the radio is already transmitting - left queued to retry next tick.
    void trySendPending() {
        if (m_pendingSendLength == 0) return;
        if (m_radio->startTransmit(m_pendingSendBuffer, m_pendingSendLength)) {
            m_pendingSendLength = 0;
        }
    }

    // Anchor is set when a downlink finishes decoding, i.e. right as the flight computer's TX ends - so
    // the danger zone is the LATTER part of the cycle (approaching its next TX), not the first half.
    // ~543ms on-air of 1000ms leaves ~457ms safe; 35% margin covers clock drift, not tuned precisely.
    bool inSafeUplinkWindow(uint32_t nowMs) const {
        if (m_downlinkCadenceMs == 0) return true; // nothing learned yet to predict from - don't block on it
        const uint32_t cyclePos = (nowMs - m_lastDownlinkRxTimeMs) % m_downlinkCadenceMs;
        return cyclePos < (m_downlinkCadenceMs * SAFE_WINDOW_FRACTION_PERCENT) / 100;
    }

    // Hex-encodes `length` bytes of `data` into `outHex` (must be at least length*2+1 bytes) - used to
    // report raw, possibly-non-printable radio payloads over the text-based debug stream.
    static void bytesToHex(const uint8_t* data, uint8_t length, char* outHex) {
        static const char digits[] = "0123456789ABCDEF";
        for (uint8_t i = 0; i < length; i++) {
            outHex[i * 2] = digits[data[i] >> 4];
            outHex[i * 2 + 1] = digits[data[i] & 0x0F];
        }
        outHex[length * 2] = '\0';
    }

    static constexpr uint32_t GPS_REPORT_INTERVAL_MS = 1000;
    static constexpr uint32_t SAFE_WINDOW_FRACTION_PERCENT = 35;
    // Comfortably more than one radioTransmitDelay cycle, so GPS reporting only resumes once a
    // response burst has genuinely stopped, not between two of its own chunks.
    static constexpr uint32_t CLI_RESPONSE_QUIET_MS = 2000;

    ArgumentFlag<const char*> m_sendFlag;
    BaseFlag* m_sendGroup[1] = {&m_sendFlag};

    RadioLink* m_radio = nullptr;
    GPS* m_gps = nullptr;
    Alarm m_gpsReportTimer;

    // Half-duplex, shared channel with the flight computer's downlink - --send queues its payload and
    // fires at a predicted safe window (learned cadence), not immediately/reactively, so bad downlink rx can't block uplink.
    char m_pendingSendBuffer[255] = {}; // matches RadioLink::RadioMessage::data / RADIOLIB_SX126X_MAX_PACKET_LENGTH
    uint32_t m_pendingSendLength = 0; // 0 = nothing queued
    // Anchor (last confirmed downlink) + cadence predict future downlink times - resynced each rx, but sends don't wait on one.
    uint32_t m_lastDownlinkRxTimeMs = 0; // 0 = no downlink received yet this session
    uint32_t m_downlinkCadenceMs = 0; // 0 = not yet learned (need at least one interval)
    uint32_t m_lastCliResponseRxTimeMs = 0; // 0 = no CLI response chunk received yet this session
};

#endif //GROUNDSTATIONRELAY_H
