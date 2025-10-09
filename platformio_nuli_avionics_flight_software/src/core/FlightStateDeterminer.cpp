#include "FlightStateDeterminer.h"
#include "HardwareAbstraction.h"

constexpr ConfigurationID_t FlightStateDeterminer::REQUIRED_CONFIGS[];

void FlightStateDeterminer::setup(Configuration* configuration) {
    m_configuration = configuration;
    m_flightState = m_configuration->getConfigurable<FLIGHT_STATE_c>();
    m_groundElevation = m_configuration->getConfigurable<GROUND_ELEVATION_c>();
    // @todo reboot detection
    // For now we assume that flight always is PRE_FLIGHT on boot
    m_flightState.set(PRE_FLIGHT);
}

FlightState_e FlightStateDeterminer::loopOnce(const State1D_s& state1D, const Timestamp_s& timestamp) {
    if (m_flightState.get() == PRE_FLIGHT) {
        // Update ground altitude reference: only update at maximum once per second, and if it has changed by over 2
        m_lowPass.update(state1D.unfilteredNoOffsetAltitudeM);
        if (timestamp.runtime_ms - m_internalSecondaryTimer > 1000) {
            if (abs(m_groundElevation.get() - m_lowPass.value()) > 2.0f) {
                m_groundElevation.set(m_lowPass.value());
            }
            m_internalSecondaryTimer = timestamp.runtime_ms;
        }

        if (hasLaunched(state1D, timestamp) && timestamp.runtime_ms > 5000) { // @todo do something smarter than waiting 5s
            m_flightState.set(ASCENT);
            m_flightStateStartTime = timestamp.runtime_ms;
        }
    } else if (m_flightState.get() == ASCENT) {
        // Track maximum altitude
        if (state1D.altitudeM > m_maxAltitude) {
            m_maxAltitude = state1D.altitudeM;
        }
        // Check if we have reached apogee
        if (apogeeReached(state1D, timestamp)) {
            m_flightState.set(DESCENT);
            m_flightStateStartTime = timestamp.runtime_ms;
        }
    } else if (m_flightState.get() == DESCENT) {
        if (hasLanded(state1D, timestamp)) {
            m_flightState.set(POST_FLIGHT);
            m_flightStateStartTime = timestamp.runtime_ms;
        }
    } else if (m_flightState.get() == POST_FLIGHT) {} else {
        m_flightState.set(PRE_FLIGHT);
        m_flightStateStartTime = timestamp.runtime_ms;
    }

    return getState();
}

bool FlightStateDeterminer::hasLaunched(const State1D_s& state1D, const Timestamp_s& timestamp) {
    // We have launched if we have seen a sufficiently high acceleration for enough time
    if (state1D.accelerationMSS > LAUNCH_ACCELERATION_THRESHOLD_MSS) {
        if (timestamp.runtime_ms - m_internalStateTransitionTimer > LAUNCH_ACCELERATION_DEBOUNCE_TIMER_MS) {
            m_internalStateTransitionTimer = timestamp.runtime_ms;
            return true;
        }
    } else {
        m_internalStateTransitionTimer = timestamp.runtime_ms;
    }

    // We have also launched if we have reached a high enough altitude
    // Note that the ground reference recalculation creates a race condition here:
    // Pose is measured relative to the ground, and the ground reference is updated in PRE_FLIGHT
    // This means that if the ground reference was updated every tick to be the current altitude, this would never trigger
    // @todo perhaps fix the race condition, and add debounce?
    if (state1D.altitudeM > LAUNCH_ALTITUDE_THRESHOLD_M) {
        return true;
    }

    return false;
}

bool FlightStateDeterminer::apogeeReached(const State1D_s& state1D, const Timestamp_s& timestamp) {
    // @todo perhaps add a altitude below max seen check
    if (state1D.velocityMS < 0) { // If we are going down
        if (timestamp.runtime_ms - m_internalStateTransitionTimer > APOGEE_DEBOUNCE_TIMER_MS) {
            m_internalStateTransitionTimer = timestamp.runtime_ms;
            if (state1D.altitudeM < m_maxAltitude - APOGEE_ALTITUDE_CHANGE_THRESHOLD_M) {
                return true;
            }
        }
    } else {
        m_internalStateTransitionTimer = timestamp.runtime_ms;
    }
    return false;
}

bool FlightStateDeterminer::hasLanded(const State1D_s& state1D, const Timestamp_s& timestamp) {
    if (fabs(m_landingDetectionReferenceAltitude - state1D.altitudeM) > LANDING_ALTITUDE_CHANGE_THRESHOLD_M) {
        m_landingDetectionReferenceAltitude = state1D.altitudeM;
        m_internalStateTransitionTimer = timestamp.runtime_ms;
    } else {
        if (timestamp.runtime_ms - m_internalStateTransitionTimer > LANDING_DEBOUNCE_TIMER_MS) {
            return true;
        }
    }
    return false;
}

FlightState_e FlightStateDeterminer::getState() const {
    return FlightState_e(m_flightState.get());
}

uint32_t FlightStateDeterminer::getStateStartTime() const {
    return m_flightStateStartTime;
}
