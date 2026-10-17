#include "UBloxV2.h"

// A lost setAutoPVT/setAutoDOP ACK would leave read()'s getters on their blocking-poll path
static constexpr uint8_t AUTO_REPORT_ENABLE_RETRIES = 3;

// Only used if auto-reporting drops out: bounds the blocking poll (8 getters * 5 ms << 100 ms watchdog)
static constexpr uint16_t GPS_GETTER_MAX_WAIT_MS = 5;

UBloxV2::UBloxV2(HardwareSerial* serial, uint8_t navigationRateHz, uint32_t baudRate) :
    m_serial(serial), m_navigationRateHz(navigationRateHz), m_baudRate(baudRate) {}

namespace {
    // Library default 1100 ms (retried up to 3x) could block setup() for tens of seconds
    constexpr uint16_t GPS_CONFIG_MAX_WAIT_MS = 150;
}

bool UBloxV2::connectAtBaud(uint32_t baud) {
    m_serial->end();
    m_serial->begin(baud);
    return m_gps.begin(*m_serial, GPS_CONFIG_MAX_WAIT_MS);
}

void UBloxV2::setup(DebugStream* debugStream, WatchdogTimer* watchdog) {
    m_debugStream = debugStream;

    // The module keeps its baud across MCU resets; 9600 is the factory default
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

    m_gps.setUART1Output(COM_TYPE_UBX, GPS_CONFIG_MAX_WAIT_MS);
    m_gps.setDynamicModel(DYN_MODEL_AIRBORNE4g, GPS_CONFIG_MAX_WAIT_MS);
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

    debugStream->message("UBloxV2 GPS initialized");
}

void UBloxV2::read() {
    m_gps.checkUblox();

    m_fixQuality = m_gps.getFixType(GPS_GETTER_MAX_WAIT_MS);
    m_satellitesTracked = m_gps.getSIV(GPS_GETTER_MAX_WAIT_MS);
    m_hdop = m_gps.getHorizontalDOP(GPS_GETTER_MAX_WAIT_MS); // scaled by 100
    m_vdop = m_gps.getVerticalDOP(GPS_GETTER_MAX_WAIT_MS);

    m_coordinates.latitudeDeg = (float)(m_gps.getLatitude(GPS_GETTER_MAX_WAIT_MS) / 10000000.0);
    m_coordinates.longitudeDeg = (float)(m_gps.getLongitude(GPS_GETTER_MAX_WAIT_MS) / 10000000.0);
    m_coordinates.altitudeM = (float)(m_gps.getAltitudeMSL(GPS_GETTER_MAX_WAIT_MS) / 1000.0);

    m_unixTimeS = m_gps.getUnixEpoch(GPS_GETTER_MAX_WAIT_MS);
}
