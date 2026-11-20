#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UBLOXM10_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UBLOXM10_H

#include "core/generic_hardware/GenericHardware.h"
#include <Arduino.h>

class UBloxM10 : public GPS {
public:
    explicit UBloxM10(HardwareSerial* serial, uint8_t navigationRateHz = 5, uint32_t baudRate = 38400);

    void setup(DebugStream* debugStream, WatchdogTimer* watchdog) override;

    void read() override;

protected:
    static constexpr uint16_t UBX_MAX_PAYLOAD_SIZE = 100;

    bool connectAtBaud(uint32_t baud);

    void sendConfiguration(bool includeBaudRate);

    void sendNavigationRate();

    void sendValset(const uint32_t* keys, const uint32_t* values, uint8_t count);

    bool waitForAck(uint16_t maxWaitMs);

    bool parseByte(uint8_t byte);

    void handleMessage();

    HardwareSerial* m_serial;
    uint8_t m_navigationRateHz;
    uint32_t m_baudRate;
    DebugStream* m_debugStream = nullptr;
    bool m_connected = false;
    uint32_t m_lastPvtMs = 0;
    uint32_t m_lastConfigureMs = 0;

    uint8_t m_parseState = 0;
    uint8_t m_messageClass = 0;
    uint8_t m_messageId = 0;
    uint16_t m_messageLength = 0;
    uint16_t m_messageIndex = 0;
    uint8_t m_checksumA = 0;
    uint8_t m_checksumB = 0;
    uint8_t m_payload[UBX_MAX_PAYLOAD_SIZE] = {};

    bool m_valsetAcked = false;
    bool m_valsetNaked = false;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UBLOXM10_H
