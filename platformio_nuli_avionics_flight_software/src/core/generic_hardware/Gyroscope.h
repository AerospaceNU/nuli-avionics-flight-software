#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GYROSCOPE_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GYROSCOPE_H

#include <Avionics.h>
#include "GenericSensor.h"

/**
 * @class Gyroscope
 * @brief Generic Gyroscope
 * @details Base class for all Gyroscopes. Can also be used as an "injected" class, where an external source provides the data (i.e. desktop sim)
 */
class Gyroscope : public GenericSensor {
public:
    /**
     * @brief Injects sensor data directly
     * @details If a sensor can't be directly read from, you can inject data directly to the class
     * @param velocitiesRadS Angular velocities in Rad/s
     * @param temperatureK Temperature in kelvin
     */
    inline void inject(Vector3D_s velocitiesRadS, float temperatureK) {
        m_velocitiesRadS = velocitiesRadS;
        m_temperatureK = temperatureK;
    }

    /**
     * @brief Gets the angular velocities
     * @return Angular velocities in Rad/s
     */
    inline Vector3D_s getVelocitiesRadS() const {
        return m_velocitiesRadS;
    }

    /**
     * @brief Gets the temperature
     * @return Temperature in K
     */
    inline float getTemperatureK() const {
        return m_temperatureK;
    }

protected:
    Vector3D_s m_velocitiesRadS = {};               ///< Sensor data vector
    float m_temperatureK = 0;                      ///< Sensor temperature
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GYROSCOPE_H
