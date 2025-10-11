#include "FlightStateDeterminer.h"

#include <delay.h>

#include "HardwareAbstraction.h"

constexpr ConfigurationID_t FlightStateDeterminer::REQUIRED_CONFIGS[];

void FlightStateDeterminer::setup(Configuration* configuration) {
    m_configuration = configuration;
    m_flightState = m_configuration->getConfigurable<FLIGHT_STATE_c>();

    // We need to determine the initial "boot" state
    constexpr Timestamp_s bootTimestamp = {0, 0, 0};
    // If a flight was completed, we can assume that we are not currently in the air
    if (getFlightState() == POST_FLIGHT) {
        setFlightState(bootTimestamp, PRE_FLIGHT);
    }
    // If we have any other state that's not PRE_FLIGHT or POST_FLIGHT, we can no longer be sure that we are in that state.
    if (getFlightState() != PRE_FLIGHT) {
        setFlightState(bootTimestamp, UNKNOWN_FLIGHT_STATE);
    }
}

FlightState_e FlightStateDeterminer::loopOnce(const Timestamp_s& timestamp, const State1D_s& state1D) {
    if (getFlightState() == PRE_FLIGHT) {
        if (hasLaunched(timestamp, state1D)) {
            setFlightState(timestamp, ASCENT);
        }
    } else if (getFlightState() == ASCENT) {
        // Track maximum altitude
        if (state1D.altitudeM > m_maxAltitude) {
            m_maxAltitude = state1D.altitudeM;
        }
        // Check if we have reached apogee
        if (apogeeReached(timestamp, state1D)) {
            setFlightState(timestamp, DESCENT);
        }
    } else if (getFlightState() == DESCENT) {
        if (hasLanded(timestamp, state1D)) {
            setFlightState(timestamp, POST_FLIGHT);
        }
    } else if (getFlightState() == POST_FLIGHT) {} else {
        // @todo update detection logic
        setFlightState(timestamp, PRE_FLIGHT);
    }

    return getFlightState();
}

bool FlightStateDeterminer::hasLaunched(const Timestamp_s& timestamp, const State1D_s& state1D) {
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

bool FlightStateDeterminer::apogeeReached(const Timestamp_s& timestamp, const State1D_s& state1D) {
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

bool FlightStateDeterminer::hasLanded(const Timestamp_s& timestamp, const State1D_s& state1D) {
    if (std::fabs(m_landingDetectionReferenceAltitude - state1D.altitudeM) > LANDING_ALTITUDE_CHANGE_THRESHOLD_M) {
        m_landingDetectionReferenceAltitude = state1D.altitudeM;
        m_internalStateTransitionTimer = timestamp.runtime_ms;
    } else {
        if (timestamp.runtime_ms - m_internalStateTransitionTimer > LANDING_DEBOUNCE_TIMER_MS) {
            return true;
        }
    }
    return false;
}

FlightState_e FlightStateDeterminer::getFlightState() const {
    return FlightState_e(m_flightState.get());
}


void FlightStateDeterminer::setFlightState(const Timestamp_s& timestamp, const FlightState_e& flightState) {
    m_flightState.set(flightState);
    m_internalStateTransitionTimer = timestamp.runtime_ms;
}

uint32_t FlightStateDeterminer::getStateStartTime() const {
    return m_flightStateStartTime;
}
