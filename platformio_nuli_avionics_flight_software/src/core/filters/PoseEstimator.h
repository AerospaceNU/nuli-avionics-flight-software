#ifndef POSEESTIMATER_H
#define POSEESTIMATER_H

#include "Avionics.h"
#include "core/HardwareAbstraction.h"
#include "core/Configuration.h"


class PoseEstimator {
public:
    constexpr static ConfigurationID_e REQUIRED_CONFIGS[] = {STATE};


    void setup(HardwareAbstraction *hardware, Configuration *configuration);

    Pose_s loopOnce();

    Pose_s getPose() const;

private:
    float getPressurePa() const;

    Vector3D_s getAccelerationMSS() const;

    static float calculateAltitudeM(float pressurePa) ;

    Pose_s m_currentPose = {};
    HardwareAbstraction *m_hardware = nullptr;
    Configuration *m_configuration = nullptr;

    ConfigurationData<int32_t> *m_state = nullptr;
};



#endif //POSEESTIMATER_H
