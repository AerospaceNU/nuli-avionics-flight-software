#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MAGNETOMETER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MAGNETOMETER_H

#include <Avionics.h>
#include "GenericSensor.h"
#include "../transform/Vector3DTransform.h"

/**
 * @class Magnetometer
 * @brief Generic Magnetometer
 * @details Base class for all Magnetometer. Can also be used as an "injected" class, where an external source provides the data (i.e. desktop sim)
 */
class Magnetometer : public GenericSensor {
public:
    explicit Magnetometer(const Vector3DTransform *transform) : m_transform(transform) {};
    /**
     * @brief Injects sensor data directly
     * @details If a sensor can't be directly read from, you can inject data directly to the class
     * @param magneticFieldTesla Magnetic field in gauss
     * @param temperatureK Temperature in kelvin
     */
    void inject(const Vector3D_s& magneticFieldTesla, const double temperatureK) {
        m_magneticFieldTesla_sensor = magneticFieldTesla;
        m_temperatureK = temperatureK;
        m_magneticFieldTesla_board = m_transform->transform(m_magneticFieldTesla_sensor);
    }

    /**
     * @brief Gets the Magnetic field
     * @return Magnetic field in gauss
     */
    Vector3D_s getMagneticFieldTesla_sensor() const {
        return m_magneticFieldTesla_sensor;
    }


    /**
     * @brief Gets the Magnetic field
     * @return Magnetic field in gauss
     */
    Vector3D_s getMagneticFieldTesla_board() const {
        return m_magneticFieldTesla_board;
    }

    /**
     * @brief Gets the temperature
     * @return Temperature in K
     */
    double getTemperatureK() const {
        return m_temperatureK;
    }

protected:
    const Vector3DTransform *m_transform;
    Vector3D_s m_magneticFieldTesla_sensor = {}; ///< Sensor data vector
    Vector3D_s m_magneticFieldTesla_board = {}; ///< Sensor data vector
    double m_temperatureK = 0; ///< Sensor temperature
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MAGNETOMETER_H
