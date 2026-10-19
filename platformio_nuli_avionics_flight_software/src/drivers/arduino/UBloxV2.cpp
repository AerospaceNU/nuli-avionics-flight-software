#include "UBloxV2.h"

// Retries for setAutoPVT/setAutoDOP at setup - a lost ACK (e.g. right after the baud-rate switch in
// connectAtBaud) would otherwise silently leave read()'s getters falling back to their blocking-poll
// path every tick, which GPS_GETTER_MAX_WAIT_MS below bounds but shouldn't have to rely on.
static constexpr uint8_t AUTO_REPORT_ENABLE_RETRIES = 3;

static constexpr uint16_t GPS_GETTER_MAX_WAIT_MS = 5; ///< bounds each blocking poll (only used if auto-reporting could not be enabled)

UBloxV2::UBloxV2(HardwareSerial* serial, uint8_t navigationRateHz, uint32_t baudRate) :
    m_serial(serial), m_navigationRateHz(navigationRateHz), m_baudRate(baudRate) {}

namespace {
    // Library defaults to 1100ms maxWait, retried up to 3x in places - unbounded, setup() could block
    // tens of seconds if the module is slow to ACK. 150ms is generous for UART; caps the worst case at a few seconds.
    constexpr uint16_t GPS_CONFIG_MAX_WAIT_MS = 150;
}

bool UBloxV2::connectAtBaud(uint32_t baud) {
    m_serial->end();
    m_serial->begin(baud);
    return m_gps.begin(*m_serial, GPS_CONFIG_MAX_WAIT_MS);
}

void UBloxV2::setup(DebugStream* debugStream, WatchdogTimer* watchdog) {
    m_debugStream = debugStream;

    // Target baud is the module's own persisted setting (survives MCU reset/reflash) - try it first;
    // fall back to factory-default 9600 (and reconfigure) only if that fails, i.e. a fresh module.
    if (!connectAtBaud(m_baudRate)) {
        if (!connectAtBaud(9600)) {
            debugStream->error("UBloxV2 GPS failed to respond at %lu or 9600 baud", (unsigned long)m_baudRate);
            return;
        }

        m_gps.setSerialRate(m_baudRate);
        if (!connectAtBaud(m_baudRate)) {
            debugStream->error("UBloxV2 GPS failed to respond after switching to %lu baud", (unsigned long)m_baudRate);
            return;
        }
    }

    m_connected = true;
    m_gps.setUART1Output(COM_TYPE_UBX, GPS_CONFIG_MAX_WAIT_MS); // Only speak UBX binary, no need for the module to also spend time on NMEA
    m_gps.setDynamicModel(DYN_MODEL_AIRBORNE4g, GPS_CONFIG_MAX_WAIT_MS); // Best suited for the high acceleration/altitude profile of a rocket flight
    if (!m_gps.setNavigationFrequency(m_navigationRateHz, GPS_CONFIG_MAX_WAIT_MS)) {
        debugStream->warn("UBloxV2 GPS rejected %u Hz navigation rate", m_navigationRateHz);
    }
    // Auto-reporting keeps read() from polling; NAV-PVT and NAV-DOP must be enabled separately
    bool autoPvtEnabled = false;
    for (uint8_t attempt = 0; attempt < AUTO_REPORT_ENABLE_RETRIES && !autoPvtEnabled; attempt++) {
        autoPvtEnabled = m_gps.setAutoPVT(true, GPS_CONFIG_MAX_WAIT_MS);
    }
    if (!autoPvtEnabled) {
        debugStream->error("UBloxV2 GPS failed to enable auto-PVT reporting after %u attempts - reads will fall back to a bounded blocking poll", (unsigned)AUTO_REPORT_ENABLE_RETRIES);
    }

    bool autoDopEnabled = false;
    for (uint8_t attempt = 0; attempt < AUTO_REPORT_ENABLE_RETRIES && !autoDopEnabled; attempt++) {
        autoDopEnabled = m_gps.setAutoDOP(true, GPS_CONFIG_MAX_WAIT_MS);
    }
    if (!autoDopEnabled) {
        debugStream->error("UBloxV2 GPS failed to enable auto-DOP reporting after %u attempts - reads will fall back to a bounded blocking poll", (unsigned)AUTO_REPORT_ENABLE_RETRIES);
    }

    m_autoReporting = autoPvtEnabled && autoDopEnabled;
    debugStream->message("UBloxV2 GPS initialized");
}

void UBloxV2::read() {
    if (!m_connected) return;
    m_gps.checkUblox();

    for (uint8_t i = 0; i < 8; i++) {
        switch (m_pollIndex) {
        case 0: m_fixQuality = m_gps.getFixType(GPS_GETTER_MAX_WAIT_MS); break;
        case 1: m_satellitesTracked = m_gps.getSIV(GPS_GETTER_MAX_WAIT_MS); break;
        case 2: m_hdop = m_gps.getHorizontalDOP(GPS_GETTER_MAX_WAIT_MS); break; // raw, scaled by 100 - matches the module's native representation
        case 3: m_vdop = m_gps.getVerticalDOP(GPS_GETTER_MAX_WAIT_MS); break;
        case 4: m_coordinates.latitudeDeg = (float)(m_gps.getLatitude(GPS_GETTER_MAX_WAIT_MS) / 10000000.0); break;
        case 5: m_coordinates.longitudeDeg = (float)(m_gps.getLongitude(GPS_GETTER_MAX_WAIT_MS) / 10000000.0); break;
        case 6: m_coordinates.altitudeM = (float)(m_gps.getAltitudeMSL(GPS_GETTER_MAX_WAIT_MS) / 1000.0); break;
        default: m_unixTimeS = m_gps.getUnixEpoch(GPS_GETTER_MAX_WAIT_MS); break;
        }
        m_pollIndex = (uint8_t)((m_pollIndex + 1) % 8);
        if (!m_autoReporting) break;
    }
}
