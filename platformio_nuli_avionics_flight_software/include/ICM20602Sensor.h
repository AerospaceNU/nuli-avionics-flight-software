#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20602Sensor_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20602Sensor_H

#include <Avionics.h>
#include <GenericSensor.h>
#include <Accelerometer.h>
#include <Gyroscope.h>
#include <Magnetometer.h>


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
    void setup() final;

    /**
     * @brief Reads data from the sensor
     * @details This uses injector classes, meaning that the data is read in, but then passed into "dummy" classes.
     * These "dummy" classes are what are actually used by the rest of the code to access data.
     */
    void read() final;

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
    // Abstraction
    Accelerometer m_accelerometer;          ///< Injector class used to pass data to the HardwareAbstraction
    Gyroscope m_gyroscope;                  ///< Injector class used to pass data to the HardwareAbstraction
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20602Sensor_H
