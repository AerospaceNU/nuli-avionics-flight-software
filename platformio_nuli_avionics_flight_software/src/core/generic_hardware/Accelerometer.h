#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ACCELEROMETER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ACCELEROMETER_H

#include <Avionics.h>
#include "GenericSensor.h"
#include "core/transform/Vector3DTransform.h"

/** @brief Accelerometer base class; also usable directly with inject() for externally supplied data (e.g. desktop sim). */
class Accelerometer : public GenericSensor {
public:
    explicit Accelerometer(const Vector3DTransform *transform) : m_transform(transform) {};

    /** @brief Sets the sensor-frame reading directly and updates the board-frame value. */
    void inject(const Vector3D_s& accelerationsMSS, const float temperatureK) {
        m_accelerationsMSS_sensor = accelerationsMSS;
        m_temperatureK = temperatureK;
        m_accelerationsMSS_board = m_transform->transform(m_accelerationsMSS_sensor);
    }

    Vector3D_s getAccelerationsMSS_sensor() const {
        return m_accelerationsMSS_sensor;
    }

    Vector3D_s getAccelerationsMSS_board() const {
        return m_accelerationsMSS_board;
    }

    float getTemperatureK() const {
        return m_temperatureK;
    }

    float getFullScaleMSS() const {
        return m_fullScaleMSS;
    }

protected:
    const Vector3DTransform *m_transform;
    Vector3D_s m_accelerationsMSS_sensor = {};
    Vector3D_s m_accelerationsMSS_board = {};

    float m_temperatureK = 0;
    float m_fullScaleMSS = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ACCELEROMETER_H
