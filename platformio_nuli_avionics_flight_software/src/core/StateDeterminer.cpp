#include "StateDeterminer.h"
#include "HardwareAbstraction.h"

constexpr ConfigurationID_e StateDeterminer::REQUIRED_CONFIGS[];

void StateDeterminer::setup(Configuration* configuration) {
    m_configuration = configuration;
    m_state = m_configuration->getConfigurable<STATE>();
    // @todo reboot detection
    // For now we assume that flight always is PRE_FLIGHT on boot
    m_state.set(PRE_FLIGHT);
}

State_e StateDeterminer::loopOnce(const Pose_s& pose) {
    switch (m_state.get()) {
    case PRE_FLIGHT: {
        /** @todo
         * Save a timestamp from GPS? nah this should probably be handeled elsewhere
         * Log occasonaly? probably worth it
         * Calculate ground pressure
         * Update orientation reference
         */

        if (hasLaunched(pose)) {
            m_state.set(ASCENT);
            m_stateStartTime = pose.timestamp_ms;
        }
    }

    case ASCENT: {
        if (apogeeReached(pose)) {
            m_state.set(DESCENT);
            m_stateStartTime = pose.timestamp_ms;
        }
    }

    case DESCENT: {
        if (hasLanded(pose)) {
            m_state.set(POST_FLIGHT);
            m_stateStartTime = pose.timestamp_ms;
        }
    }

    case POST_FLIGHT: {}

    default: {
        m_state.set(PRE_FLIGHT);
        m_stateStartTime = pose.timestamp_ms;
    }
    }

    return getState();
}

bool StateDeterminer::hasLaunched(const Pose_s& pose) {
    // We have launched if we have seen a sufficiently high acceleration for enough time
    if (pose.acceleration.z > LAUNCH_ACCELERATION_THRESHOLD_MSS) {
        if (pose.timestamp_ms - m_internalStateTransitionTimer > LAUNCH_ACCELERATION_DEBOUNCE_TIMER_MS) {
            m_internalStateTransitionTimer = pose.timestamp_ms;
            return true;
        }
    } else {
        m_internalStateTransitionTimer = pose.timestamp_ms;
    }

    // We have also launched if we have reached a high enough altitude
    // Note that the ground reference recalculation creates a race condition here:
    // Pose is measured relative to the ground, and the ground reference is updated in PRE_FLIGHT
    // This means that if the ground reference was updated every tick to be the current altitude, this would never trigger
    // @todo perhaps fix the race condition, and add debounce?
    if (pose.position.z > LAUNCH_ALTITUDE_THRESHOLD_M) {
        return true;
    }

    return false;
}

bool StateDeterminer::apogeeReached(const Pose_s& pose) {
    if (pose.velocity.z < 0) { // If we are going down
        if (pose.timestamp_ms - m_internalStateTransitionTimer > APOGEE_DEBOUNCE_TIMER_MS) {
            m_internalStateTransitionTimer = pose.timestamp_ms;
            return true;
        }
    } else {
        m_internalStateTransitionTimer = pose.timestamp_ms;
    }
    return false;
}

bool StateDeterminer::hasLanded(const Pose_s& pose) {
    if (fabs(m_landingDetectionReferenceAltitude - pose.position.z) > LANDING_ALTITUDE_CHANGE_THRESHOLD_M) {
        m_landingDetectionReferenceAltitude = pose.position.z;
        m_internalStateTransitionTimer = pose.timestamp_ms;
    } else {
        if (pose.timestamp_ms - m_internalStateTransitionTimer > LANDING_DEBOUNCE_TIMER_MS) {
            return true;
        }
    }
    return false;
}

State_e StateDeterminer::getState() const {
    return State_e(m_state.get());
}

uint32_t StateDeterminer::getStateStartTime() const {
    return m_stateStartTime;
}

