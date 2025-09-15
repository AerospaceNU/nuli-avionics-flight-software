#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "Avionics.h"
#include "Configuration.h"
#include "ConfigurationRegistry.h"

class StateDeterminer {
public:
    constexpr static ConfigurationID_e REQUIRED_CONFIGS[] = {STATE};

    void setup(Configuration* configuration);

    State_e loopOnce(const Pose_s& pose);

    State_e getState() const;

    uint32_t getStateStartTime() const;

private:
    bool hasLaunched(const Pose_s& pose);

    bool apogeeReached(const Pose_s& pose);

    bool hasLanded(const Pose_s& pose);

    ConfigurationData<int32_t> m_state;
    Configuration* m_configuration = nullptr;

    uint32_t m_internalStateTransitionTimer = 0;
    double m_landingDetectionReferenceAltitude = 0;

    uint32_t m_stateStartTime = 0;
};

#endif //STATEMACHINE_H
