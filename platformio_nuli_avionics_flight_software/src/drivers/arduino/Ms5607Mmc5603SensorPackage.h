#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS5607MMC5603SENSORPACKAGE_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS5607MMC5603SENSORPACKAGE_H

#include <Avionics.h>
#include "core/generic_hardware/GenericHardware.h"
#include "core/transform/Vector3DTransform.h"

/**
 * @brief MS5607 barometer + ICM20602/ICM42605 IMU + optional MMC5603NJ magnetometer on one I2C bus
 * @details SillyGoose V1: ICM20602, no mag. SillyGoose V2: ICM42605, no mag. SeriousGoose: ICM42605 + mag.
 * @note Pressure conversion (OSR 2048, ~4.54 ms) starts at the end of one read() and is normally done by the next;
 * read() waits out any remainder, then runs temperature (shared ADC) before restarting pressure.
 */
class Ms5607Mmc5603SensorPackage final : public GenericSensor {
public:
    /** @brief Which IMU chip is populated on this board - register maps and protocols differ. */
    enum class ImuType : uint8_t {
        ICM20602, ///< SillyGoose V1
        ICM42605, ///< SillyGoose V2, SeriousGoose
    };

    /**
     * @param hasMagnetometer If false, the MMC5603NJ is never accessed and getMagnetometer() returns nullptr
     * @param magTransform Defaults to transform
     */
    explicit Ms5607Mmc5603SensorPackage(const Vector3DTransform* transform, ImuType imuType, bool hasMagnetometer, const Vector3DTransform* magTransform = nullptr);

    /** @brief Clears the I2C bus, initializes the sensors, and starts the first pressure conversion */
    void setup(DebugStream* debugStream, WatchdogTimer* watchdog) override;

    /** @brief Reads every sensor; blocks at most ~4.54 ms + ~600 us. On a failed transfer the sensor keeps its last reading and the bus is cleared. */
    void read() override;

    Accelerometer* getAccelerometer();

    Gyroscope* getGyroscope();

    Barometer* getBarometer();

    /** @return nullptr if constructed with hasMagnetometer = false */
    Magnetometer* getMagnetometer();

private:
    // ---- IMU (ICM20602 or ICM42605, chosen by m_imuType) ----
    void setupImu();
    bool readAndInjectImu();

    void setupImuIcm20602();
    bool readAndInjectImuIcm20602();

    void setupImuIcm42605();
    bool readAndInjectImuIcm42605();

    // ---- Barometer (MS5607) ----
    void setupBarometer();
    bool readBarometerCalibration();
    bool triggerPressureConversion();
    bool triggerTemperatureConversion();
    bool readBarometerAdc(uint32_t& value) const;
    void waitForPressureConversion() const;
    void waitForTemperatureConversion() const;
    void computeAndInjectBarometerReading(uint32_t rawPressure, uint32_t rawTemperature);

    // ---- Magnetometer (MMC5603NJ) ----
    void setupMagnetometer();
    bool readAndInjectMag();

    // ---- I2C bus recovery ----
    void recoverBus();

    // ---- Injector instances exposed to HardwareAbstraction ----
    Accelerometer m_accelerometer;
    Gyroscope m_gyroscope;
    Barometer m_barometer;
    Magnetometer m_magnetometer;
    const ImuType m_imuType;
    const bool m_hasMagnetometer;
    DebugStream* m_debugStream = nullptr;

    // ---- IMU raw data ----
    int16_t m_accelData[3] = {};
    int16_t m_gyroData[3] = {};
    int16_t m_imuTempRaw = 0;

    // ---- Barometer calibration (read from PROM at setup) and conversion state ----
    uint16_t m_baroC1 = 0, m_baroC2 = 0, m_baroC3 = 0, m_baroC4 = 0, m_baroC5 = 0, m_baroC6 = 0;
    bool m_baroCalibrated = false;
    bool m_busFaulted = false;
    uint32_t m_busRecoveries = 0;
    // Start time of whichever conversion is in flight (pressure, except briefly inside read())
    uint32_t m_baroConversionStartUs = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS5607MMC5603SENSORPACKAGE_H
