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

private:
    bool hasLaunched(const Pose_s& pose);

    bool apogeeReached(const Pose_s& pose);

    bool hasLanded(const Pose_s& pose);

    ConfigurationData<int32_t>* m_state = nullptr;
    Configuration* m_configuration = nullptr;

    uint32_t m_stateTransitionTimer = 0;
    double altitude = 0;
};

#endif //STATEMACHINE_H
