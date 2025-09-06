#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "Avionics.h"
#include "Configuration.h"
#include "ConfigurationRegistry.h"

class StateMachine {
public:
    void setup(Configuration *configuration);

    void update(const Pose_s& pose);

    bool hasLaunched(const Pose_s& pose);

    bool apogeeReached(const Pose_s& pose);

    bool hasLanded(const Pose_s& pose);

private:
    ConfigurationData<int32_t> * m_state = nullptr;
    Configuration * m_configuration = nullptr;
};

#endif //STATEMACHINE_H
