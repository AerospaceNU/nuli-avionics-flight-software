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
    const bool aboveLaunchAcceleration = state1D.accelerationMSS >= LAUNCH_ACCELERATION_THRESHOLD_MSS;
    // We have also launched if we have reached a high enough altitude
    // Note that the ground reference recalculation creates a race condition here:
    // Pose is measured relative to the ground, and the ground reference is updated in PRE_FLIGHT
    // This means that if the ground reference was updated every tick to be the current altitude, this would never trigger.
    // A low pass filter with a low constant is used in StateEstimator1D to allow the race condition to work out.
    const bool aboveLaunchAltitude = state1D.altitudeM >= LAUNCH_ALTITUDE_THRESHOLD_M;
    // Run the debounce check
    if (launchDebounce.check(aboveLaunchAcceleration || aboveLaunchAltitude, timestamp)) {
        return true;
    }
    return false;
}

bool FlightStateDeterminer::apogeeReached(const Timestamp_s& timestamp, const State1D_s& state1D) {
    // Track maximum altitude
    if (state1D.altitudeM > m_maxAltitude) {
        m_maxAltitude = state1D.altitudeM;
    }
    // Run debounce checks
    const bool goingDown = state1D.velocityMS < 0;
    const bool belowApogeeChangeThreshold = state1D.altitudeM < m_maxAltitude - APOGEE_ALTITUDE_CHANGE_THRESHOLD_M;
    if (apogeeDebounce.check(goingDown && belowApogeeChangeThreshold, timestamp)) {
        return true;
    }
    return false;
}

bool FlightStateDeterminer::hasLanded(const Timestamp_s& timestamp, const State1D_s& state1D) {
    const bool aboveAltitudeChangedThreshold = std::fabs(m_landingDetectionReferenceAltitude - state1D.altitudeM) > LANDING_ALTITUDE_CHANGE_THRESHOLD_M;
    if (aboveAltitudeChangedThreshold) {
        m_landingDetectionReferenceAltitude = state1D.altitudeM;
    }
    if (landingDebounce.check(!aboveAltitudeChangedThreshold, timestamp)) {
        return true;
    }
    return false;
}

FlightState_e FlightStateDeterminer::getFlightState() const {
    return FlightState_e(m_flightState.get());
}


void FlightStateDeterminer::setFlightState(const Timestamp_s& timestamp, const FlightState_e& flightState) {
    m_flightState.set(flightState);
    m_flightStateStartTime = timestamp.runtime_ms;
}

uint32_t FlightStateDeterminer::getStateStartTime() const {
    return m_flightStateStartTime;
}
