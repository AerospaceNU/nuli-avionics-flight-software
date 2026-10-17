#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GYROSCOPE_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GYROSCOPE_H

#include <Avionics.h>
#include "GenericSensor.h"
#include "core/transform/Vector3DTransform.h"

/** @brief Gyroscope base class; also usable directly with inject() for externally supplied data (e.g. desktop sim). */
class Gyroscope : public GenericSensor {
public:
    explicit Gyroscope(const Vector3DTransform *transform) : m_transform(transform) {};


    /** @brief Sets the raw sensor-frame reading directly and updates the bias-removed board-frame value. */
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

    /** @brief Sensor frame, bias not removed. */
    Vector3D_s getVelocitiesRadS_raw() const {
        return m_velocitiesRadS_raw;
    }

    Vector3D_s getVelocitiesRadS_board_biasRemoved() const {
        return m_velocitiesRadS_board_biasRemoved;
    }

    float getTemperatureK() const {
        return m_temperatureK;
    }

protected:
    const Vector3DTransform *m_transform;
    Vector3D_s m_velocitiesRadS_raw = {};
    Vector3D_s m_velocitiesRadS_board_biasRemoved = {};
    Vector3D_s m_biasOffset = {};               ///< sensor frame
    float m_temperatureK = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GYROSCOPE_H

