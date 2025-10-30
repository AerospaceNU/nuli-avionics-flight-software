#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GYROSCOPE_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GYROSCOPE_H

#include <Avionics.h>
#include "GenericSensor.h"
#include "../transform/Vector3DTransform.h"

/**
 * @class Gyroscope
 * @brief Generic Gyroscope
 * @details Base class for all Gyroscopes. Can also be used as an "injected" class, where an external source provides the data (i.e. desktop sim)
 */
class Gyroscope : public GenericSensor {
public:
    explicit Gyroscope(const Vector3DTransform *transform) : m_transform(transform) {};


    /**
     * @brief Injects sensor data directly
     * @details If a sensor can't be directly read from, you can inject data directly to the class
     * @param velocitiesRadS Angular velocities in Rad/s
     * @param temperatureK Temperature in kelvin
     */
    void inject(Vector3D_s velocitiesRadS, const float temperatureK) {
        m_velocitiesRadS_raw = velocitiesRadS;
        m_temperatureK = temperatureK;
        velocitiesRadS.x = velocitiesRadS.x - m_biasOffset.x;
        velocitiesRadS.y = velocitiesRadS.y - m_biasOffset.y;
        velocitiesRadS.z = velocitiesRadS.z - m_biasOffset.z;
        m_velocitiesRadS_board_biasRemoved = m_transform->transform(velocitiesRadS);
    }

    void setBiasOffset(const Vector3D_s& offset) {
        m_biasOffset = offset;
    }

    /**
     * @brief Gets the angular velocities
     * @return Angular velocities in Rad/s
     */
    Vector3D_s getVelocitiesRadS_raw() const {
        return m_velocitiesRadS_raw;
    }

    /**
     * @brief Gets the angular velocities
     * @return Angular velocities in Rad/s
     */
    Vector3D_s getVelocitiesRadS_board_biasRemoved() const {
        return m_velocitiesRadS_board_biasRemoved;
    }

    /**
     * @brief Gets the temperature
     * @return Temperature in K
     */
    float getTemperatureK() const {
        return m_temperatureK;
    }

protected:
    const Vector3DTransform *m_transform;
    Vector3D_s m_velocitiesRadS_raw = {};               ///< Sensor data vector
    Vector3D_s m_velocitiesRadS_board_biasRemoved = {};               ///< Sensor data vector
    Vector3D_s m_biasOffset = {};               ///< Sensor data vector
    float m_temperatureK = 0;                      ///< Sensor temperature
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GYROSCOPE_H

