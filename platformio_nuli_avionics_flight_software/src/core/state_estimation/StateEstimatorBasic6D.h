#ifndef STATEESTIMATORBASIC6D_H
#define STATEESTIMATORBASIC6D_H

#include "../filters/KalmanFilter1D.h"
#include "Avionics.h"
#include "../configuration/Configuration.h"
#include "ConstantsUnits.h"
#include "core/generic_hardware/Accelerometer.h"
#include "core/generic_hardware/Gyroscope.h"
#include "core/generic_hardware/Barometer.h"
#include "../filters/LowPass.h"



class StateEstimatorBasic6D {
public:
    StateEstimatorBasic6D(bool useKalman);

    void setup(HardwareAbstraction* hardware, Configuration* configuration);

    State6D_s update(const Timestamp_s& timestamp, const State1D_s& state1D, const Orientation_s& orientation);

    State6D_s getState6D() const;

private:
    Vector3D_s getAccelerationMSS(const Orientation_s& orientation) const;

    Vector3D_s projectVelocities(const Orientation_s& orientation, float velocityZ) const;

    bool m_useKalman;

    HardwareAbstraction* m_hardware = nullptr;
    Configuration* m_configuration = nullptr;
    DebugStream* m_debug = nullptr;

    State6D_s m_currentState6D = {};
    ConfigurationData<float> m_groundElevation;
};


#endif //STATEESTIMATORBASIC6D_H
