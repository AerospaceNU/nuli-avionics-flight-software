#ifndef ORIENTATIONESTIMATOR_H
#define ORIENTATIONESTIMATOR_H

#include "Avionics.h"
#include "core/configuration/Configuration.h"
#include "core/generic_hardware/GenericHardware.h"
#include "core/filters/LowPass.h"
#include "util/Timer.h"

class OrientationEstimator {
public:
    constexpr static ConfigurationID_t REQUIRED_CONFIGS[] = {GYROSCOPE_BIAS_c, LAUNCH_ANGLE_c};

    void setup(HardwareAbstraction* hardware, Configuration* configuration);

    const Orientation_s& update(const Timestamp_s& timestamp, const FlightState_e& flightState);

    const Orientation_s& getOrientation() const;

private:
    Quaternion integrateGyroscope(const Timestamp_s& timestamp, const Vector3D_s& angularVelocity) const;

    Quaternion updateLaunchAngle(const Timestamp_s& timestamp);

    void updateGyroscopeBias(const Timestamp_s& timestamp);

    float computeTilt(const Quaternion& q) const;

    void quaternionToEuler(const Quaternion& q);

    HardwareAbstraction* m_hardware = nullptr;
    Configuration* m_configuration = nullptr;
    DebugStream* m_debug = nullptr;

    Orientation_s m_currentOrientation = {};

    ConfigurationData<Quaternion> m_launchAngle;
    Alarm m_launchAngleAlarm;
    LowPass m_launchAngleLowPassX{0.01};
    LowPass m_launchAngleLowPassY{0.01};
    LowPass m_launchAngleLowPassZ{0.01};

    ConfigurationData<GyroscopeBias_s> m_gyroscopeBias;
    ConfigurationData<int32_t> m_boardOrientation;

    struct LowPass3D_s {
        LowPass x{0.0005};
        LowPass y{0.0005};
        LowPass z{0.0005};
        Vector3D_s lastVelocity{0, 0, 0};
    };

    LowPass3D_s m_lowPass[MAX_GYROSCOPE_NUM];
    Debounce m_motionDetector{250};
    Alarm m_gyroscopeBiasAlarm;
};


#endif //ORIENTATIONESTIMATOR_H
