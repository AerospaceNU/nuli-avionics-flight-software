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
    inline void inject(Vector3D_s accelerationsMSS, double temperatureK) {
        m_accelerationsMSS = accelerationsMSS;
        m_temperatureK = temperatureK;
    }

    /**
     * @brief Gets the Accelerations
     * @return Accelerations in m/s^2
     */
    inline Vector3D_s getAccelerationsMSS() const {
        return m_accelerationsMSS;
    }

    /**
     * @brief Gets the temperature
     * @return Temperature in K
     */
    inline double getTemperatureK() const {
        return m_temperatureK;
    }

protected:
    Vector3D_s m_accelerationsMSS = {};         ///< Sensor data vector
    double m_temperatureK = 0;                  ///< Sensor temperature
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ACCELEROMETER_H
