#ifndef GROUNDSTATIONRELAY_H
#define GROUNDSTATIONRELAY_H

#include "Avionics.h"
#include "cli/Parser.h"
#include "cli/ArgumentFlag.h"
#include "generic_hardware/RadioLink.h"
#include "generic_hardware/GPS.h"
#include "util/Timer.h"
#include <cstring>

/** @brief Ground-station mode (GROUND_STATION_MODE_c): relays radio telemetry to USB, reports own GPS, uplinks --send. Caller sets up the radio and GPS. */
class GroundStationRelay {
public:
    GroundStationRelay() : m_sendFlag("--send", "Queues a radio uplink payload", true, [this](DebugStream* debugStream) { this->sendCallback(debugStream); }) {}

    void setup(Parser* parser, RadioLink* radio, GPS* gps) {
        m_radio = radio;
        m_gps = gps;
        parser->addFlagGroup(m_sendGroup);
        m_gpsReportTimer.startAlarm(0, 0);
    }

    // Call every loop in place of the flight-computer radio handling
    void tick(const Timestamp_s& timestamp, DebugStream* debugStream) {
        // GPS lines are held off while a CLI response is arriving so they don't split one of its lines
        const bool receivingCliResponse = m_lastCliResponseRxTimeMs != 0 &&
            (timestamp.runtime_ms - m_lastCliResponseRxTimeMs) < CLI_RESPONSE_QUIET_MS;
        if (!receivingCliResponse && m_gpsReportTimer.isAlarmFinished(timestamp.runtime_ms)) {
            const Coordinates_s coords = m_gps->getCoordinates();
            debugStream->data("GPS\t%.6f\t%.6f\t%.2f\t%lu\t%u\t%u\t%d\t%d",
                coords.latitudeDeg, coords.longitudeDeg, coords.altitudeM,
                m_gps->getUnixTimeS(), m_gps->getHDOP(), m_gps->getVDOP(), m_gps->getFixQuality(), m_gps->getSatellitesTracked());
            m_gpsReportTimer.startAlarm(timestamp.runtime_ms, GPS_REPORT_INTERVAL_MS);
        }

        // Packets go to USB hex-encoded; the host tool decodes them
        if (m_radio->isMessageAvailable()) {
            const RadioLink::RadioMessage message = m_radio->readMessage();

            if (m_lastDownlinkRxTimeMs != 0) {
                const uint32_t rawInterval = timestamp.runtime_ms - m_lastDownlinkRxTimeMs;
                uint32_t impliedCadence = rawInterval;
                if (m_downlinkCadenceMs > 0) {
                    // A gap of ~N cadences means N-1 dropped packets: divide by the nearest multiple
                    uint32_t multiple = (rawInterval + m_downlinkCadenceMs / 2) / m_downlinkCadenceMs;
                    if (multiple == 0) multiple = 1;
                    impliedCadence = rawInterval / multiple;
                }
                // 3:1 IIR smoothing
                m_downlinkCadenceMs = (m_downlinkCadenceMs == 0) ? impliedCadence : (m_downlinkCadenceMs * 3 + impliedCadence) / 4;
            }
            m_lastDownlinkRxTimeMs = timestamp.runtime_ms;
            // Before the USB print: a slow host could eat into the ~457ms safe window
            trySendPending();

            if (message.length >= 1 && message.data[0] == RADIO_MSG_CLI_RESPONSE) {
                m_lastCliResponseRxTimeMs = timestamp.runtime_ms;
                // Raw pass-through: already the flying board's CLI text, newlines included
                debugStream->writeRaw(message.data + 1, message.length - 1);
            } else {
                char hexBuffer[sizeof(message.data) * 2 + 1];
                bytesToHex(message.data, message.length, hexBuffer);
                debugStream->data("RADIO_RX\t%d\t%.2f\t%s", message.rssi, message.snr, hexBuffer);
            }
        }

        // Time-based, so uplink still goes out when downlink packets are lost
        if (m_pendingSendLength > 0 && inSafeUplinkWindow(timestamp.runtime_ms)) {
            trySendPending();
        }
    }

private:
    void sendCallback(DebugStream* debugStream) {
        const char* payload = m_sendFlag.getValueDerived();
        // No null terminator: the receiver's pushIncoming() adds its own, and a second one is a phantom empty command
        uint32_t len = (uint32_t)strlen(payload);
        if (len > sizeof(m_pendingSendBuffer) - 1) len = sizeof(m_pendingSendBuffer) - 1;
        m_pendingSendBuffer[0] = RADIO_MSG_CLI_COMMAND;
        memcpy(m_pendingSendBuffer + 1, payload, len);
        m_pendingSendLength = len + 1;
        debugStream->message("Uplink queued (%lu bytes) - sending at the next predicted safe window", (unsigned long)len);
    }

    // Fails only if the radio is already transmitting; stays queued for the next tick
    void trySendPending() {
        if (m_pendingSendLength == 0) return;
        if (m_radio->startTransmit(m_pendingSendBuffer, m_pendingSendLength)) {
            m_pendingSendLength = 0;
        }
    }

    // Cycle starts as a downlink TX ends, so the safe part is the start; ~543ms on-air of 1000ms, 35% leaves drift margin
    bool inSafeUplinkWindow(uint32_t nowMs) const {
        if (m_downlinkCadenceMs == 0) return true;
        const uint32_t cyclePos = (nowMs - m_lastDownlinkRxTimeMs) % m_downlinkCadenceMs;
        return cyclePos < (m_downlinkCadenceMs * SAFE_WINDOW_FRACTION_PERCENT) / 100;
    }

    // outHex must hold length*2+1 bytes
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
    // More than one radioTransmitDelay cycle, so GPS doesn't resume between response chunks
    static constexpr uint32_t CLI_RESPONSE_QUIET_MS = 2000;

    ArgumentFlag<const char*> m_sendFlag;
    BaseFlag* m_sendGroup[1] = {&m_sendFlag};

    RadioLink* m_radio = nullptr;
    GPS* m_gps = nullptr;
    Alarm m_gpsReportTimer;

    // Half-duplex channel shared with the downlink: --send waits for a predicted safe window
    char m_pendingSendBuffer[255] = {}; // RADIOLIB_SX126X_MAX_PACKET_LENGTH
    uint32_t m_pendingSendLength = 0;
    // 0 in the next three = none yet
    uint32_t m_lastDownlinkRxTimeMs = 0;
    uint32_t m_downlinkCadenceMs = 0;
    uint32_t m_lastCliResponseRxTimeMs = 0;
};

#endif //GROUNDSTATIONRELAY_H
