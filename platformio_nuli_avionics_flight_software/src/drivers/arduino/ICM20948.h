#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20948_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20948_H

#include <Avionics.h>
#include <GenericSensor.h>
#include <Accelerometer.h>
#include <Gyroscope.h>
#include <Magnetometer.h>
#include <ICM_20948.h>

/**
 * @class ICM20948
 * @brief Driver for ICM20948
 * @details Uses the sparkfun Arduino ICM20948 library, and abstracts it to our sensor formats
 */
class ICM20948 final : public GenericSensor {
public:
    /**
     * @brief Creates a ICM20948
     * @param chipSelectPin Chip select pin for the SPI bus
     */
    explicit ICM20948(uint8_t chipSelectPin);

    /**
     * @brief Sets the SPI object to use
     * @param spiClass SPI object to use for communication with the sensor
     */
    void setSpiClass(SPIClass* spiClass);

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
     * The ICM20948 is basically a 3 in one sensor, so we make a "dummy" class for each of the 3 sensors, and pass data through it.
     * @return Accelerometer injector class
     */
    Accelerometer* getAccelerometer();

    /**
     * @brief Gets the Gyroscope injector class
     * @details Injector classes are used to pass data to the HardwareAbstraction.
     * The ICM20948 is basically a 3 in one sensor, so we make a "dummy" class for each of the 3 sensors, and pass data through it.
     * @return Gyroscope injector class
     */
    Gyroscope* getGyroscope();

    /**
     * @brief Gets the Magnetometer injector class
     * @details Injector classes are used to pass data to the HardwareAbstraction.
     * The ICM20948 is basically a 3 in one sensor, so we make a "dummy" class for each of the 3 sensors, and pass data through it.
     * @return Magnetometer injector class
     */
    Magnetometer* getMagnetometer();

private:
    // Hardware facing
    const uint8_t m_chipSelectPin;          ///< The chip select pin for the SPI bus
    SPIClass* m_spiClass = nullptr;         ///< Which SPI bus to use
    ICM_20948_SPI sparkfunIcm20948;         ///< The library object which handles registers/read/write/all the messy stuff
    // Abstraction
    Accelerometer m_accelerometer;          ///< Injector class used to pass data to the HardwareAbstraction
    Gyroscope m_gyroscope;                  ///< Injector class used to pass data to the HardwareAbstraction
    Magnetometer m_magnetometer;            ///< Injector class used to pass data to the HardwareAbstraction
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM20948_H
