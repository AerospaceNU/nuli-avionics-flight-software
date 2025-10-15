#ifndef STATEESTIMATORBASIC6D_H
#define STATEESTIMATORBASIC6D_H

#include "Avionics.h"
#include "core/Configuration.h"
#include "ConstantsUnits.h"
#include "core/generic_hardware/Accelerometer.h"
#include "core/generic_hardware/Gyroscope.h"
#include "core/generic_hardware/Barometer.h"

class StateEstimatorBasic6D {
public:
    void setup(HardwareAbstraction* hardware, Configuration* configuration);

    State6D_s loopOnce(const Timestamp_s& timestamp, const State1D_s &state1D, const FlightState_e &flightState);

    State6D_s getState6D() const;

private:
    Vector3D_s integrateGyroscope(const Vector3D_s& angularVelocity) const;

    Vector3D_s projectVelocityToWorld(const Vector3D_s& euler, float bodyVelocity) const;

    Vector3D_s projectAccelToWorld(const Vector3D_s& euler, const Vector3D_s& accelBody) const;

    HardwareAbstraction* m_hardware = nullptr;
    Configuration* m_configuration = nullptr;

    State6D_s m_currentState6D= {};

    ConfigurationData<float> m_groundElevation;
    ConfigurationData<int32_t> m_boardOrientation;

    float m_xPos = 0;
    float m_yPos = 0;
};



#endif //STATEESTIMATORBASIC6D_H
