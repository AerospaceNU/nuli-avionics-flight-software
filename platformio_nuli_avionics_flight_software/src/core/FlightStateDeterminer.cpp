#include "FlightStateDeterminer.h"

#include <delay.h>

#include "HardwareAbstraction.h"
#include "Arduino.h"
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
    if (getFlightState() == ASCENT || getFlightState() == DESCENT) {
        setFlightState(bootTimestamp, UNKNOWN_FLIGHT_STATE);
    }

    // This should never happen, but if it does: we assume we are in PRE_FLIGHT
    if (getFlightState() != UNKNOWN_FLIGHT_STATE && getFlightState() != PRE_FLIGHT) {
        setFlightState(bootTimestamp, PRE_FLIGHT);
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
    } else if (getFlightState() == POST_FLIGHT) {
        // Nothing to do
    } else if (getFlightState() == UNKNOWN_FLIGHT_STATE) {
        m_unknownStateVelocityLowPass.update(state1D.velocityMS);
        if (state1D.altitudeM > m_unknownStateMaximumAltitude) {
            m_unknownStateMaximumAltitude = state1D.altitudeM;
        }
        if (state1D.altitudeM < m_unknownStateMinimumAltitude) {
            m_unknownStateMinimumAltitude = state1D.altitudeM;
        }
        if (m_unknownStateTimer.check(true, timestamp.runtime_ms)) {
            const bool sufficientDeltaAltitude = m_unknownStateMaximumAltitude - m_unknownStateMinimumAltitude > UNKNOWN_STATE_ALTITUDE_CHANGE_THRESHOLD_M;
            const bool sufficientVelocity = std::fabs(m_unknownStateVelocityLowPass.value()) > UNKNOWN_STATE_VELOCITY_THRESHOLD_MS;
            if (sufficientDeltaAltitude && sufficientVelocity) {
                setFlightState(timestamp, ASCENT); // Always set to ascent to because ASCENT will rapidly determine if in ASCENT or DESCENT
            } else {
                setFlightState(timestamp, PRE_FLIGHT);
            }
            // Reset in case we ever use the state again, but this shouldn't happen
            m_unknownStateTimer.reset();
            m_unknownStateVelocityLowPass.reset();
            m_unknownStateMaximumAltitude = -999999999.0f;
            m_unknownStateMinimumAltitude = 99999999.0f;
        }
    } else {
        setFlightState(timestamp, UNKNOWN_FLIGHT_STATE);
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
    if (m_launchDebounce.check(aboveLaunchAcceleration || aboveLaunchAltitude, timestamp.runtime_ms)) {
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
    if (m_apogeeDebounce.check(goingDown && belowApogeeChangeThreshold, timestamp.runtime_ms)) {
        return true;
    }
    return false;
}

bool FlightStateDeterminer::hasLanded(const Timestamp_s& timestamp, const State1D_s& state1D) {
    const bool aboveAltitudeChangedThreshold = std::fabs(m_landingDetectionReferenceAltitude - state1D.altitudeM) > LANDING_ALTITUDE_CHANGE_THRESHOLD_M;
    if (aboveAltitudeChangedThreshold) {
        m_landingDetectionReferenceAltitude = state1D.altitudeM;
    }
    if (m_landingDebounce.check(!aboveAltitudeChangedThreshold, timestamp.runtime_ms)) {
        return true;
    }
    return false;
}

FlightState_e FlightStateDeterminer::getFlightState() const {
    return FlightState_e(m_flightState.get());
}


void FlightStateDeterminer::setFlightState(const Timestamp_s& timestamp, const FlightState_e& flightState) {
    m_flightState.set(flightState);
    m_stateStopWatch.startWatch(timestamp.runtime_ms);
}

const StopWatch* FlightStateDeterminer::getStateTimer() const {
    return &m_stateStopWatch;
}
