#ifndef ORIENTATIONESTIMATOR_H
#define ORIENTATIONESTIMATOR_H

#include "../../Avionics.h"
#include "../configuration/Configuration.h"
#include "../../ConstantsUnits.h"
#include "../generic_hardware/Accelerometer.h"
#include "../generic_hardware/Gyroscope.h"
#include "../transform/Quaternion.h"
#include "../filters/LowPass.h"
#include "util/Timer.h"

class OrientationEstimator {
public:
    constexpr static ConfigurationID_t REQUIRED_CONFIGS[] = {GYROSCOPE_BIAS_c, LAUNCH_ANGLE_c};

    void setup(HardwareAbstraction* hardware, Configuration* configuration);

    const Orientation_s& update(const Timestamp_s& timestamp, const FlightState_e& flightState);

    const Orientation_s& getOrientation() const;

private:
    void integrateGyroscope(const Timestamp_s& timestamp, const Vector3D_s& angularVelocity);

    void updateLaunchAngle(const Timestamp_s& timestamp);

    void updateGyroscopeBias(const Timestamp_s& timestamp);

    HardwareAbstraction* m_hardware = nullptr;
    Configuration* m_configuration = nullptr;
    DebugStream* m_debug = nullptr;

    Orientation_s m_currentOrientation = {};

    ConfigurationData<Quaternion_s> m_launchAngle;
    Alarm m_launchAngleAlarm;
    LowPass m_launchAngleLowPassX{0.01};
    LowPass m_launchAngleLowPassY{0.01};
    LowPass m_launchAngleLowPassZ{0.01};

    ConfigurationData<GyroscopeBias_s> m_gyroscopeBias;
    struct LowPass3D_s {
        LowPass x{0.001};
        LowPass y{0.001};
        LowPass z{0.001};
    };
    LowPass3D_s m_lowPass[MAX_GYROSCOPE_NUM];
    Alarm m_gyroscopeBiasAlarm;

};


#endif //ORIENTATIONESTIMATOR_H
