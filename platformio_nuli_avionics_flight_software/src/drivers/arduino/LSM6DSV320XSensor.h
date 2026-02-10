#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LSM6DSV320XSENSOR_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LSM6DSV320XSENSOR_H

#include <Arduino.h>
#include <Avionics.h>
#include <SPI.h>

#include "../../core/generic_hardware/GenericSensor.h"
#include "core/generic_hardware/Accelerometer.h"
#include "core/generic_hardware/Gyroscope.h"
#include "core/transform/Vector3DTransform.h"




/**
 * @class LSM6DSV320X Driver
 * @brief driver for LSM6DSV320X
 * @details driver for SPI connection only
 */
class LSM6DSV320XSensor final : public GenericSensor {
public:

    /**
     * @brief create a LSM6DSV320X Object
     * @param transform -> transform into board coordinates
     */
    explicit LSM6DSV320XSensor(const Vector3DTransform *transform, const uint8_t chipSelectPin, SPIClass *spi);

    /**
     * @brief Initializes the sensor
     * @details Starts communication, and configures parameters
     */
    void setup(DebugStream *debugStream) override;

    /**
     * @brief Reads data from the sensor
     * @details This uses injector classes, meaning that the data is read in, but then passed into "dummy" classes.
     * These "dummy" classes are what are actually used by the rest of the code to access data.
     */
    void read() override;

    /**
     * @brief Gets the High G Accelerometer injector class
     * @details Injector classes are used to pass data to the HardwareAbstraction.
     * The LSM is basically a 3 in one sensor, so we make a "dummy" class for each of the 3 sensors, and pass data through it.
     * @return High G Accelerometer injector class
     */
    Accelerometer* getHighGAccelerometer();

    /**
     * @brief Gets the Low G Accelerometer injector class
     * @details Injector classes are used to pass data to the HardwareAbstraction.
     * The LSM is basically a 3 in one sensor, so we make a "dummy" class for each of the 3 sensors, and pass data through it.
     * @return Low G Accelerometer injector class
     */
    Accelerometer* getLowGAccelerometer();

    /**
     * @brief Gets the Gyroscope injector class
     * @details Injector classes are used to pass data to the HardwareAbstraction.
     * The ICM20602 is basically a 3 in one sensor, so we make a "dummy" class for each of the 3 sensors, and pass data through it.
     * @return Gyroscope injector class
     */
    Gyroscope* getGyroscope();

private:
    // Injector Classes
    Accelerometer m_highGAccelerometer;     ///< Injector class used to pass data to the HardwareAbstraction
    Accelerometer m_lowGAccelerometer;      ///< Injector class used to pass data to the HardwareAbstraction
    Gyroscope m_gyroscope;                  ///< Injector class used to pass data to the HardwareAbstraction

    //SPI
    uint8_t m_chipSelectPin;                ///< SPI chip select pin
    SPIClass *m_spi;                        ///< SPI instance passed via constructor

    // Data
    int16_t highGAccelData[3] = {};
    int16_t lowGAccelData[3] = {};
    int16_t gyroData[3] = {};
    int16_t tempRaw = 0;

    // internal functions
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    void readBytes(uint8_t reg, uint8_t *buffer, uint8_t length);
};



#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_LSM6DSV320XSENSOR_H