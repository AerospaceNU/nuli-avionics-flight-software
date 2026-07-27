#include "UBloxV2.h"

UBloxV2::UBloxV2(HardwareSerial* serial, uint8_t navigationRateHz, uint32_t baudRate) :
    m_serial(serial), m_navigationRateHz(navigationRateHz), m_baudRate(baudRate) {}

bool UBloxV2::connectAtBaud(uint32_t baud) {
    m_serial->end();
    m_serial->begin(baud);
    return m_gps.begin(*m_serial);
}

void UBloxV2::setup(DebugStream* debugStream) {
    m_debugStream = debugStream;

    // u-blox modules ship at 9600 baud by default - connect at that rate first so we can raise it
    if (!connectAtBaud(9600)) {
        debugStream->error("UBloxV2 GPS failed to respond at 9600 baud");
        return;
    }

    m_gps.setSerialRate(m_baudRate);
    if (!connectAtBaud(m_baudRate)) {
        debugStream->error("UBloxV2 GPS failed to respond after switching to %lu baud", (unsigned long)m_baudRate);
        return;
    }

    m_gps.setUART1Output(COM_TYPE_UBX); // Only speak UBX binary, no need for the module to also spend time on NMEA
    m_gps.setDynamicModel(DYN_MODEL_AIRBORNE4g); // Best suited for the high acceleration/altitude profile of a rocket flight
    if (!m_gps.setNavigationFrequency(m_navigationRateHz)) {
        debugStream->warn("UBloxV2 GPS rejected %u Hz navigation rate", m_navigationRateHz);
    }
    // Have the module push each fix automatically instead of us polling for it, so read() doesn't have to
    // block on a request/response round trip every loop at the configured navigation rate.
    // NAV-PVT (fix/lat/lon/alt/time) and NAV-DOP (HDOP) are separate messages and need auto-reporting enabled
    // independently - without setAutoDOP, getHorizontalDOP() falls back to a blocking poll on every read().
    m_gps.setAutoPVT(true);
    m_gps.setAutoDOP(true);

    debugStream->message("UBloxV2 GPS initialized");
}

void UBloxV2::read() {
    m_gps.checkUblox();

    m_fixQuality = m_gps.getFixType();
    m_satellitesTracked = m_gps.getSIV();
    m_hdop = m_gps.getHorizontalDOP(); // raw, scaled by 100 - matches the module's native representation
    m_vdop = m_gps.getVerticalDOP();

    m_coordinates.latitudeDeg = (float)(m_gps.getLatitude() / 10000000.0);
    m_coordinates.longitudeDeg = (float)(m_gps.getLongitude() / 10000000.0);
    m_coordinates.altitudeM = (float)(m_gps.getAltitudeMSL() / 1000.0);

    m_unixTimeS = m_gps.getUnixEpoch();
}
