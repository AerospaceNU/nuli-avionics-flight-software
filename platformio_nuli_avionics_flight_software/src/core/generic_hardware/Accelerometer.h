#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ACCELEROMETER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ACCELEROMETER_H

#include <Avionics.h>
#include "GenericSensor.h"

/**
 * @class Accelerometer
 * @brief Generic Accelerometer
 * @details Base class for all Accelerometer. Can also be used as an "injected" class, where an external source provides the data (i.e. desktop sim)
 */
class Accelerometer : public GenericSensor {
public:
    /**
     * @brief Injects sensor data directly
     * @details If a sensor can't be directly read from, you can inject data directly to the class
     * @param accelerationsMSS Accelerations in m/s^2
     * @param temperatureK Temperature in kelvin
     */
    void inject(const Vector3D_s& accelerationsMSS, const float temperatureK) {
        m_accelerationsMSS = accelerationsMSS;
        m_temperatureK = temperatureK;
    }

    /**
     * @brief Gets the Accelerations
     * @return Accelerations in m/s^2
     */
    Vector3D_s getAccelerationsMSS() const {
        return m_accelerationsMSS;
    }

    /**
     * @brief Gets the temperature
     * @return Temperature in K
     */
    float getTemperatureK() const {
        return m_temperatureK;
    }

    /**
     * @brief Gets the sensor's full scale reading
     * @return Full scale reading in m/s^2
     */
    float getFullScaleMSS() const {
        return m_fullScaleMSS;
    }

protected:
    Vector3D_s m_accelerationsMSS = {}; ///< Sensor data vector
    float m_temperatureK = 0; ///< Sensor temperature
    float m_fullScaleMSS = 0; ///< Sensor full scale reading
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ACCELEROMETER_H
