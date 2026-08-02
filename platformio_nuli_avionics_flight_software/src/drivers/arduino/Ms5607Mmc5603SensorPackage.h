#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS5607MMC5603SENSORPACKAGE_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS5607MMC5603SENSORPACKAGE_H

#include <Avionics.h>
#include "core/generic_hardware/GenericHardware.h"
#include "core/transform/Vector3DTransform.h"

/**
 * @class Ms5607Mmc5603SensorPackage
 * @brief Barometer (MS5607) + selectable IMU (ICM20602/ICM42605) + optional magnetometer (MMC5603NJ), one I2C bus.
 * @details Covers every board: SillyGoose V1 (ICM20602, no mag), SillyGoose V2/SeriousGoose (ICM42605, mag on SeriousGoose only).
 * @note Pressure conversion (OSR 2048, ~4.54ms) is too long to block a 100Hz loop, so it's started at the end of one read() and normally already done by the next - read() waits out any remainder rather than skipping the barometer, then resolves temperature (shares the ADC) before restarting pressure - every sensor resolves every tick. OSR 4096 was tried first but left only ~0.96ms margin against the ~10ms tick, which the ~1.3ms temperature cycle didn't fit - pressure only resolved every other tick as a result.
 */
class Ms5607Mmc5603SensorPackage final : public GenericSensor {
public:
    /** @brief Which IMU chip is populated on this board - register maps and protocols differ. */
    enum class ImuType : uint8_t {
        ICM20602, ///< SillyGoose V1
        ICM42605, ///< SillyGoose V2, SeriousGoose
    };

    /**
     * @brief Creates the sensor package.
     * @param transform Orientation transform for the accelerometer and gyroscope.
     * @param imuType Which IMU chip is populated on this board.
     * @param hasMagnetometer Whether an MMC5603NJ magnetometer is populated - if false, it's never talked to over I2C, and getMagnetometer() returns nullptr.
     * @param magTransform Orientation transform for the magnetometer; defaults to `transform` if the magnetometer isn't mounted at a different rotation than the IMU.
     */
    explicit Ms5607Mmc5603SensorPackage(const Vector3DTransform* transform, ImuType imuType, bool hasMagnetometer, const Vector3DTransform* magTransform = nullptr);

    /** @brief Initializes the IMU, barometer, and (if present) magnetometer, and starts the first barometer conversion. */
    void setup(DebugStream* debugStream, WatchdogTimer* watchdog) override;

    /** @brief Reads the IMU, magnetometer (if present), and barometer every call - waits out any remaining pressure conversion time rather than skipping it, then resolves pressure+temperature and restarts the conversion (see class comment). Normally instant; bounded to ~4.54ms + ~600us worst case. */
    void read() override;

    /** @return the accelerometer injector class, for registering with HardwareAbstraction */
    Accelerometer* getAccelerometer();

    /** @return the gyroscope injector class, for registering with HardwareAbstraction */
    Gyroscope* getGyroscope();

    /** @return the barometer injector class, for registering with HardwareAbstraction */
    Barometer* getBarometer();

    /** @return the magnetometer injector class, for registering with HardwareAbstraction, or nullptr if constructed with hasMagnetometer = false. */
    Magnetometer* getMagnetometer();

private:
    // ---- IMU (ICM20602 or ICM42605, chosen by m_imuType) ----
    void setupImu();
    void readAndInjectImu();

    void setupImuIcm20602();
    void readAndInjectImuIcm20602();

    void setupImuIcm42605();
    void readAndInjectImuIcm42605();

    // ---- Barometer (MS5607) ----
    void setupBarometer();
    bool readBarometerCalibration();
    void triggerPressureConversion();
    void triggerTemperatureConversion();
    uint32_t readBarometerAdc() const;
    void waitForPressureConversion() const;
    void waitForTemperatureConversion() const;
    void computeAndInjectBarometerReading(uint32_t rawPressure, uint32_t rawTemperature);

    // ---- Magnetometer (MMC5603NJ) ----
    void setupMagnetometer();
    void readAndInjectMag();

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
    // Start time of whichever conversion is in flight. Always the pressure conversion at the top of
    // read() - triggerTemperatureConversion() repurposes it mid-call, but triggerPressureConversion() overwrites it again before read() returns.
    uint32_t m_baroConversionStartUs = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MS5607MMC5603SENSORPACKAGE_H
