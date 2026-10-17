#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UBLOXV2_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UBLOXV2_H

#include "core/generic_hardware/GenericHardware.h"
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

/**
 * @brief u-blox M8 GPS driver (SparkFun u-blox GNSS library v2)
 * @details M10 modules need the v3 library, which can't coexist with v2 in one image (both define SFE_UBLOX_GNSS).
 */
class UBloxV2 : public GPS {
public:
    /** @param navigationRateHz 10 Hz is the safe max for M8 modules running concurrent GNSS */
    explicit UBloxV2(HardwareSerial* serial, uint8_t navigationRateHz = 10, uint32_t baudRate = 38400);

    void setup(DebugStream* debugStream, WatchdogTimer* watchdog) override;

    void read() override;

protected:
    bool connectAtBaud(uint32_t baud);

    HardwareSerial* m_serial;
    uint8_t m_navigationRateHz;
    uint32_t m_baudRate;
    SFE_UBLOX_GNSS m_gps;
    DebugStream* m_debugStream = nullptr;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UBLOXV2_H