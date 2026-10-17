#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MAGNETOMETER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MAGNETOMETER_H

#include <Avionics.h>
#include "GenericSensor.h"
#include "core/transform/Vector3DTransform.h"

/** @brief Magnetometer base class; also usable directly with inject() for externally supplied data (e.g. desktop sim). */
class Magnetometer : public GenericSensor {
public:
    explicit Magnetometer(const Vector3DTransform *transform) : m_transform(transform) {};
    /** @brief Sets the sensor-frame reading directly and updates the board-frame value. */
    void inject(const Vector3D_s& magneticFieldTesla, const double temperatureK) {
        m_magneticFieldTesla_sensor = magneticFieldTesla;
        m_temperatureK = temperatureK;
        m_magneticFieldTesla_board = m_transform->transform(m_magneticFieldTesla_sensor);
    }

    Vector3D_s getMagneticFieldTesla_sensor() const {
        return m_magneticFieldTesla_sensor;
    }


    Vector3D_s getMagneticFieldTesla_board() const {
        return m_magneticFieldTesla_board;
    }

    double getTemperatureK() const {
        return m_temperatureK;
    }

protected:
    const Vector3DTransform *m_transform;
    Vector3D_s m_magneticFieldTesla_sensor = {};
    Vector3D_s m_magneticFieldTesla_board = {};
    double m_temperatureK = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_MAGNETOMETER_H
