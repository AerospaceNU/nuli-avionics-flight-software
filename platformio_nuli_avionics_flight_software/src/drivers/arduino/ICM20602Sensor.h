#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20602Sensor_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20602Sensor_H

#include <Avionics.h>
#include "../../core/generic_hardware/GenericSensor.h"
#include "../../core/generic_hardware/Accelerometer.h"
#include "../../core/generic_hardware/Gyroscope.h"
#include "../../core/generic_hardware/Magnetometer.h"


/**
 * @class ICM20602Sensor
 * @brief Driver for ICM20602
 * @details I2C only
 */
class ICM20602Sensor final : public GenericSensor {
public:
    /**
     * @brief Creates a ICM 20602
     */
    explicit ICM20602Sensor();

    /**
     * @brief Initializes the sensor
     * @details Starts communication, and configures parameters
     */
    void setup(DebugStream* debugStream) override;

    /**
     * @brief Reads data from the sensor
     * @details This uses injector classes, meaning that the data is read in, but then passed into "dummy" classes.
     * These "dummy" classes are what are actually used by the rest of the code to access data.
     */
    void read() override;

    /**
     * @brief Gets the Accelerometer injector class
     * @details Injector classes are used to pass data to the HardwareAbstraction.
     * The ICM20602 is basically a 3 in one sensor, so we make a "dummy" class for each of the 3 sensors, and pass data through it.
     * @return Accelerometer injector class
     */
    Accelerometer* getAccelerometer();

    /**
     * @brief Gets the Gyroscope injector class
     * @details Injector classes are used to pass data to the HardwareAbstraction.
     * The ICM20602 is basically a 3 in one sensor, so we make a "dummy" class for each of the 3 sensors, and pass data through it.
     * @return Gyroscope injector class
     */
    Gyroscope* getGyroscope();

private:
    void readAccelAndGyroBatch();

    // Abstraction
    Accelerometer m_accelerometer;          ///< Injector class used to pass data to the HardwareAbstraction
    Gyroscope m_gyroscope;                  ///< Injector class used to pass data to the HardwareAbstraction

    // For accelerometer
    int16_t accelData[3] = {};
    int16_t gyroData[3] = {};
    int16_t tempRaw = 0;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20602Sensor_H
