#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UBLOXV2_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UBLOXV2_H

#include "core/generic_hardware/GenericHardware.h"
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> // Make sure "SparkFun u-blox GNSS Arduino Library" (v2) is installed

/**
 * @class UBloxV2
 * @brief GPS driver for u-blox M8-generation modules (e.g. MAX-M8) using the SparkFun u-blox GNSS Arduino Library v2.
 * @details M10-generation modules need the newer "SparkFun u-blox GNSS v3" library instead. The two libraries both
 * declare a global `SFE_UBLOX_GNSS` class, so they can't be linked into the same firmware image - the correct
 * driver has to be picked per-board at compile time rather than autodetected at runtime.
 */
class UBloxV2 : public GPS {
public:
    /**
     * @brief Creates a u-blox M8 GPS driver
     * @param serial Serial port connected to the module
     * @param navigationRateHz Fix rate to configure the module for. 10 Hz is the safe max for M8 modules running concurrent GNSS.
     * @param baudRate Baud rate to reconfigure the module and serial port to after connecting at the module's default rate
     */
    explicit UBloxV2(HardwareSerial* serial, uint8_t navigationRateHz = 10, uint32_t baudRate = 38400);

    /**
     * @brief Initialize the sensor
     * @details Connects to the module, switches it to the configured baud rate, disables NMEA output in favor of
     * UBX-only, and configures it for high-dynamics rocket flight (airborne <4g dynamic model, max navigation rate).
     */
    void setup(DebugStream* debugStream) override;

    /**
     * @brief Read data from the sensor
     * @details Polls the module for a fresh position fix and updates fix quality/satellite/HDOP/coordinate data.
     */
    void read() override;

protected:
    /**
     * @brief (Re)connects to the module over m_serial at the given baud rate
     * @details Restarts the local serial port at the new baud and re-runs the library's handshake.
     * Needed twice during setup() - once at the module's factory-default 9600 baud, and again after
     * telling the module to switch to m_baudRate, since the local port has to be reconfigured to match.
     */
    bool connectAtBaud(uint32_t baud);

    HardwareSerial* m_serial;
    uint8_t m_navigationRateHz;
    uint32_t m_baudRate;
    SFE_UBLOX_GNSS m_gps;
    DebugStream* m_debugStream = nullptr; ///< Stashed from setup() so other methods can log too
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UBLOXV2_H