#ifndef STATEESTIMATORBASIC6D_H
#define STATEESTIMATORBASIC6D_H

#include "Avionics.h"
#include "core/Configuration.h"
#include "ConstantsUnits.h"
#include "core/generic_hardware/Accelerometer.h"
#include "core/generic_hardware/Gyroscope.h"
#include "core/generic_hardware/Barometer.h"
#include "core/transform/Quaternion.h"
#include "LowPass.h"
#include "altitude_kf.h"


class StateEstimatorBasic6D {
public:
    void setup(HardwareAbstraction* hardware, Configuration* configuration);

    State6D_s update(const Timestamp_s& timestamp, const State1D_s& state1D, const Orientation_s& orientation);

    State6D_s getState6D() const;

private:
    HardwareAbstraction* m_hardware = nullptr;
    Configuration* m_configuration = nullptr;
    DebugStream* m_debug = nullptr;

    State6D_s m_currentState6D = {};
    ConfigurationData<float> m_groundElevation;

    AltitudeKf m_kalmanFilterX;
    AltitudeKf m_kalmanFilterY;
    AltitudeKf m_kalmanFilterZ;
};


#endif //STATEESTIMATORBASIC6D_H
