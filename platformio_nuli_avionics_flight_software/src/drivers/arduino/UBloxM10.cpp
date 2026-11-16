#include "UBloxM10.h"

static constexpr uint8_t UBX_SYNC_1 = 0xB5;
static constexpr uint8_t UBX_SYNC_2 = 0x62;
static constexpr uint8_t UBX_CLASS_NAV = 0x01;
static constexpr uint8_t UBX_CLASS_ACK = 0x05;
static constexpr uint8_t UBX_CLASS_CFG = 0x06;
static constexpr uint8_t UBX_NAV_PVT = 0x07;
static constexpr uint8_t UBX_NAV_DOP = 0x04;
static constexpr uint8_t UBX_ACK_ACK = 0x01;
static constexpr uint8_t UBX_ACK_NAK = 0x00;
static constexpr uint8_t UBX_CFG_VALSET = 0x8A;
static constexpr uint8_t VALSET_LAYER_RAM = 0x01;

static constexpr uint32_t CFG_UART1_BAUDRATE = 0x40520001;
static constexpr uint32_t CFG_UART1OUTPROT_UBX = 0x10740001;
static constexpr uint32_t CFG_UART1OUTPROT_NMEA = 0x10740002;
static constexpr uint32_t CFG_NAVSPG_DYNMODEL = 0x20110021;
static constexpr uint32_t CFG_RATE_MEAS = 0x30210001;
static constexpr uint32_t CFG_MSGOUT_UBX_NAV_PVT_UART1 = 0x20910007;
static constexpr uint32_t CFG_MSGOUT_UBX_NAV_DOP_UART1 = 0x20910039;
static constexpr uint32_t DYNMODEL_AIRBORNE_4G = 8;

static constexpr uint16_t GPS_CONFIG_MAX_WAIT_MS = 150;
static constexpr uint8_t GPS_CONFIG_RETRIES = 3;
static constexpr uint32_t GPS_RECONFIGURE_AFTER_MS = 3000;
static constexpr uint16_t GPS_MAX_BYTES_PER_READ = 512;
static constexpr uint16_t NAV_PVT_LENGTH = 92;
static constexpr uint16_t NAV_DOP_LENGTH = 18;

static uint16_t readU16(const uint8_t* p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t readU32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int32_t readI32(const uint8_t* p) {
    return (int32_t)readU32(p);
}

static uint32_t unixTime(const uint16_t year, const uint8_t month, const uint8_t day, const uint8_t hour, const uint8_t minute, const uint8_t second) {
    const int32_t y = (int32_t)year - (month <= 2 ? 1 : 0);
    const int32_t era = y / 400;
    const int32_t yearOfEra = y - era * 400;
    const int32_t monthIndex = (int32_t)month + (month > 2 ? -3 : 9);
    const int32_t dayOfYear = (153 * monthIndex + 2) / 5 + (int32_t)day - 1;
    const int32_t dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    const int32_t days = era * 146097 + dayOfEra - 719468;
    return (uint32_t)days * 86400UL + (uint32_t)hour * 3600UL + (uint32_t)minute * 60UL + (uint32_t)second;
}

UBloxM10::UBloxM10(HardwareSerial* serial, uint8_t navigationRateHz, uint32_t baudRate) :
    m_serial(serial), m_navigationRateHz(navigationRateHz), m_baudRate(baudRate) {}

bool UBloxM10::connectAtBaud(const uint32_t baud) {
    m_serial->end();
    m_serial->begin(baud);
    while (m_serial->available() > 0) m_serial->read();
    m_parseState = 0;
    for (uint8_t attempt = 0; attempt < GPS_CONFIG_RETRIES; attempt++) {
        sendConfiguration(false);
        if (waitForAck(GPS_CONFIG_MAX_WAIT_MS)) return true;
    }
    return false;
}

void UBloxM10::setup(DebugStream* debugStream, WatchdogTimer* watchdog) {
    m_debugStream = debugStream;

    if (!connectAtBaud(m_baudRate)) {
        if (!connectAtBaud(9600)) {
            debugStream->error("UBloxM10 GPS failed to respond at %lu or 9600 baud", (unsigned long)m_baudRate);
            m_lastConfigureMs = millis();
            return;
        }
        sendConfiguration(true);
        m_serial->flush();
        delay(20);
        if (!connectAtBaud(m_baudRate)) {
            debugStream->error("UBloxM10 GPS failed to respond after switching to %lu baud", (unsigned long)m_baudRate);
            m_lastConfigureMs = millis();
            return;
        }
    }

    bool rateAccepted = false;
    for (uint8_t attempt = 0; attempt < GPS_CONFIG_RETRIES && !rateAccepted; attempt++) {
        sendNavigationRate();
        rateAccepted = waitForAck(GPS_CONFIG_MAX_WAIT_MS);
    }
    if (!rateAccepted) {
        debugStream->warn("UBloxM10 GPS rejected %u Hz navigation rate", m_navigationRateHz);
    }

    m_connected = true;
    m_lastConfigureMs = millis();
    m_lastPvtMs = millis();
    debugStream->message("UBloxM10 GPS initialized");
}

void UBloxM10::read() {
    for (uint16_t i = 0; i < GPS_MAX_BYTES_PER_READ && m_serial->available() > 0; i++) {
        if (parseByte((uint8_t)m_serial->read())) handleMessage();
    }

    const uint32_t now = millis();
    if (now - m_lastPvtMs > GPS_RECONFIGURE_AFTER_MS && now - m_lastConfigureMs > GPS_RECONFIGURE_AFTER_MS) {
        sendConfiguration(false);
        sendNavigationRate();
        m_lastConfigureMs = now;
    }
}

void UBloxM10::sendConfiguration(const bool includeBaudRate) {
    const uint32_t keys[] = {CFG_UART1OUTPROT_UBX, CFG_UART1OUTPROT_NMEA, CFG_NAVSPG_DYNMODEL, CFG_MSGOUT_UBX_NAV_PVT_UART1, CFG_MSGOUT_UBX_NAV_DOP_UART1, CFG_UART1_BAUDRATE};
    const uint32_t values[] = {1, 0, DYNMODEL_AIRBORNE_4G, 1, 1, m_baudRate};
    sendValset(keys, values, includeBaudRate ? 6 : 5);
}

void UBloxM10::sendNavigationRate() {
    const uint32_t keys[] = {CFG_RATE_MEAS};
    const uint32_t values[] = {(uint32_t)(1000 / (m_navigationRateHz > 0 ? m_navigationRateHz : 1))};
    sendValset(keys, values, 1);
}

void UBloxM10::sendValset(const uint32_t* keys, const uint32_t* values, const uint8_t count) {
    uint8_t message[6 + 4 + 6 * 12 + 2];
    uint16_t length = 4;
    uint8_t* payload = message + 6;
    payload[0] = 0;
    payload[1] = VALSET_LAYER_RAM;
    payload[2] = 0;
    payload[3] = 0;
    for (uint8_t k = 0; k < count && k < 6; k++) {
        const uint32_t key = keys[k];
        for (uint8_t b = 0; b < 4; b++) payload[length++] = (uint8_t)(key >> (8 * b));
        const uint8_t sizeCode = (uint8_t)((key >> 28) & 0x07);
        const uint8_t valueBytes = sizeCode == 3 ? 2 : (sizeCode == 4 ? 4 : (sizeCode == 5 ? 8 : 1));
        for (uint8_t b = 0; b < valueBytes; b++) payload[length++] = b < 4 ? (uint8_t)(values[k] >> (8 * b)) : 0;
    }
    message[0] = UBX_SYNC_1;
    message[1] = UBX_SYNC_2;
    message[2] = UBX_CLASS_CFG;
    message[3] = UBX_CFG_VALSET;
    message[4] = (uint8_t)(length & 0xFF);
    message[5] = (uint8_t)(length >> 8);
    uint8_t checksumA = 0;
    uint8_t checksumB = 0;
    for (uint16_t i = 2; i < 6 + length; i++) {
        checksumA = (uint8_t)(checksumA + message[i]);
        checksumB = (uint8_t)(checksumB + checksumA);
    }
    message[6 + length] = checksumA;
    message[7 + length] = checksumB;
    m_valsetAcked = false;
    m_valsetNaked = false;
    m_serial->write(message, 8 + length);
}

bool UBloxM10::waitForAck(const uint16_t maxWaitMs) {
    const uint32_t start = millis();
    while (millis() - start < maxWaitMs) {
        while (m_serial->available() > 0) {
            if (parseByte((uint8_t)m_serial->read())) handleMessage();
            if (m_valsetAcked) return true;
            if (m_valsetNaked) return false;
        }
    }
    return false;
}

bool UBloxM10::parseByte(const uint8_t byte) {
    switch (m_parseState) {
    case 0:
        if (byte == UBX_SYNC_1) m_parseState = 1;
        return false;
    case 1:
        m_parseState = byte == UBX_SYNC_2 ? 2 : (byte == UBX_SYNC_1 ? 1 : 0);
        return false;
    case 2:
        m_messageClass = byte;
        m_checksumA = byte;
        m_checksumB = byte;
        m_parseState = 3;
        return false;
    case 3:
        m_messageId = byte;
        m_parseState = 4;
        break;
    case 4:
        m_messageLength = byte;
        m_parseState = 5;
        break;
    case 5:
        m_messageLength = (uint16_t)(m_messageLength | (byte << 8));
        m_messageIndex = 0;
        m_parseState = m_messageLength > UBX_MAX_PAYLOAD_SIZE ? 0 : (m_messageLength == 0 ? 7 : 6);
        break;
    case 6:
        m_payload[m_messageIndex++] = byte;
        if (m_messageIndex >= m_messageLength) m_parseState = 7;
        break;
    case 7:
        m_parseState = byte == m_checksumA ? 8 : 0;
        return false;
    default:
        m_parseState = 0;
        return byte == m_checksumB;
    }
    m_checksumA = (uint8_t)(m_checksumA + byte);
    m_checksumB = (uint8_t)(m_checksumB + m_checksumA);
    return false;
}

void UBloxM10::handleMessage() {
    if (m_messageClass == UBX_CLASS_ACK && m_messageLength == 2 && m_payload[0] == UBX_CLASS_CFG && m_payload[1] == UBX_CFG_VALSET) {
        if (m_messageId == UBX_ACK_ACK) m_valsetAcked = true;
        if (m_messageId == UBX_ACK_NAK) m_valsetNaked = true;
    } else if (m_messageClass == UBX_CLASS_NAV && m_messageId == UBX_NAV_PVT && m_messageLength == NAV_PVT_LENGTH) {
        const uint8_t* p = m_payload;
        m_fixQuality = p[20];
        m_satellitesTracked = p[23];
        m_coordinates.longitudeDeg = (float)(readI32(p + 24) / 10000000.0);
        m_coordinates.latitudeDeg = (float)(readI32(p + 28) / 10000000.0);
        m_coordinates.altitudeM = (float)(readI32(p + 36) / 1000.0);
        const uint8_t valid = p[11];
        if ((valid & 0x03) == 0x03) {
            m_unixTimeS = unixTime(readU16(p + 4), p[6], p[7], p[8], p[9], p[10]);
        }
        m_connected = true;
        m_lastPvtMs = millis();
    } else if (m_messageClass == UBX_CLASS_NAV && m_messageId == UBX_NAV_DOP && m_messageLength == NAV_DOP_LENGTH) {
        m_vdop = readU16(m_payload + 10);
        m_hdop = readU16(m_payload + 12);
    }
}
