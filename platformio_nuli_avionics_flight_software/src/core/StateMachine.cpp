#include "StateMachine.h"

#include "drivers/arduino/USLI2025Payload.h"

void StateMachine::setup(Configuration* configuration) {
    m_configuration = configuration;
    m_state = m_configuration->getConfigurable<STATE>();
}

void StateMachine::update(const Pose_s& pose) {
    switch (m_state->get()) {
    case PRE_FLIGHT: {
        if (hasLaunched(pose)) {
            m_state->set(ASCENT);
        }
    }

    case ASCENT: {
        if (apogeeReached(pose)) {
            m_state->set(DESCENT);
        }
    }

    case DESCENT: {
        if (hasLanded(pose)) {
            m_state->set(POST_FLIGHT);
        }
    }

    case POST_FLIGHT: {}

    default: {
        m_state->set(PRE_FLIGHT);
    }
    }
}


bool StateMachine::hasLaunched(const Pose_s& pose) {}

bool StateMachine::apogeeReached(const Pose_s& pose) {}

bool StateMachine::hasLanded(const Pose_s& pose) {}
