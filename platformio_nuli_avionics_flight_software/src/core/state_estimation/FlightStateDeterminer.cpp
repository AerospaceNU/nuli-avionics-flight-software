#include "FlightStateDeterminer.h"

constexpr ConfigurationID_t FlightStateDeterminer::REQUIRED_CONFIGS[];

void FlightStateDeterminer::setup(Configuration* configuration) {
    m_configuration = configuration;
    m_flightState = m_configuration->getConfigurable<FLIGHT_STATE_c>();
    m_firstStageBurnout = m_configuration->getConfigurable<FIRST_STAGE_BURNOUT_c>();
    m_firstStageBurnout.set(false);
    m_machLockDebounce.check(true, 0); // primes the debounce so it can pass ASAP if mach is never reached

    constexpr Timestamp_s bootTimestamp = {0, 0, 0};

    // A completed flight means we are not in the air now
    if (getFlightState() == POST_FLIGHT) {
        setFlightState(bootTimestamp, PRE_FLIGHT);
    }
    // Rebooted mid-flight: can no longer trust that state
    m_machLockoutClearedAtBoot = getFlightState() == DESCENT;
    if (getFlightState() == ASCENT || getFlightState() == DESCENT) {
        setFlightState(bootTimestamp, UNKNOWN_FLIGHT_STATE);
    }

    // Should never happen
    if (getFlightState() != UNKNOWN_FLIGHT_STATE && getFlightState() != PRE_FLIGHT) {
        setFlightState(bootTimestamp, PRE_FLIGHT);
    }
}

FlightState_e FlightStateDeterminer::update(const Timestamp_s& timestamp, const State1D_s& state1D) {
    m_stateTransitionTick = false;
    if (getFlightState() == PRE_FLIGHT) {
        if (hasLaunched(timestamp, state1D)) {
            setFlightState(timestamp, ASCENT);
        }
    } else if (getFlightState() == ASCENT) {
        if (m_burnoutDebounce.check(state1D.accelerationMSS < 0, timestamp.runtime_ms) && !m_firstStageBurnout.get()) {
            m_firstStageBurnout.set(true);
            m_burnoutStopWatch.startWatch(timestamp.runtime_ms);
        }
        if (apogeeReached(timestamp, state1D)) {
            setFlightState(timestamp, DESCENT);
        }
        if (state1D.velocityMS > m_maxVelocity) {
            m_maxVelocity = state1D.velocityMS;
        }
        if (state1D.accelerationMSS > m_maxAcceleration) {
            m_maxAcceleration = state1D.accelerationMSS;
        }
    } else if (getFlightState() == DESCENT) {
        if (hasLanded(timestamp, state1D)) {
            setFlightState(timestamp, POST_FLIGHT);
        }
    } else if (getFlightState() == POST_FLIGHT) {
        // Nothing to do
    } else if (getFlightState() == UNKNOWN_FLIGHT_STATE) {
        if (!state1D.barometerValid) return getFlightState();
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
            const bool highAboveGround = m_unknownStateMaximumAltitude > LAUNCH_ALTITUDE_THRESHOLD_M;
            if (sufficientDeltaAltitude && sufficientVelocity) {
                setFlightState(timestamp, ASCENT); // ASCENT quickly moves on to DESCENT if already past apogee
            } else if (highAboveGround && !m_unknownStateHighTimer.check(true, timestamp.runtime_ms)) {
                return getFlightState();
            } else {
                setFlightState(timestamp, PRE_FLIGHT);
                m_machLockoutClearedAtBoot = false;
            }
            resetUnknownStateDetection();
        }
    } else {
        setFlightState(timestamp, UNKNOWN_FLIGHT_STATE);
    }
    return getFlightState();
}

bool FlightStateDeterminer::hasLaunched(const Timestamp_s& timestamp, const State1D_s& state1D) {
    if (!state1D.barometerValid) return false;
    const bool aboveLaunchAcceleration = state1D.accelerationMSS >= LAUNCH_ACCELERATION_THRESHOLD_MSS;
    // Ground reference also tracks altitude in PRE_FLIGHT; only its slow low pass in StateEstimator1D lets this trigger
    const bool aboveLaunchAltitude = state1D.altitudeM >= LAUNCH_ALTITUDE_THRESHOLD_M;
    if (m_launchDebounce.check(aboveLaunchAcceleration || aboveLaunchAltitude, timestamp.runtime_ms)) {
        return true;
    }
    return false;
}

bool FlightStateDeterminer::apogeeReached(const Timestamp_s& timestamp, const State1D_s& state1D) {
    const bool belowMachLockout = state1D.velocityMS < MACH_LOCKOUT_THRESHOLD_MS;
    if (!belowMachLockout) m_machLockoutClearedAtBoot = false;
    if (m_machLockDebounce.check(belowMachLockout, timestamp.runtime_ms) || m_machLockoutClearedAtBoot) {
        if (state1D.altitudeM > m_maxAltitude) {
            m_maxAltitude = state1D.altitudeM;
        }
        const bool goingDown = state1D.velocityMS < 0;
        const bool belowApogeeChangeThreshold = state1D.altitudeM < m_maxAltitude - APOGEE_ALTITUDE_CHANGE_THRESHOLD_M;
        if (m_apogeeDebounce.check(goingDown && belowApogeeChangeThreshold, timestamp.runtime_ms)) {
            return true;
        }
    } else {
        m_apogeeDebounce.reset();
    }
    return false;
}

bool FlightStateDeterminer::hasLanded(const Timestamp_s& timestamp, const State1D_s& state1D) {
    if (!state1D.barometerValid) return false;
    const float rawAltitudeM = m_landingRawAltitudeLowPass.update(state1D.unfilteredNoOffsetAltitudeM);
    const bool filteredChanged = std::fabs(m_landingDetectionReferenceAltitude - state1D.altitudeM) > LANDING_ALTITUDE_CHANGE_THRESHOLD_M;
    const bool rawChanged = std::fabs(m_landingDetectionReferenceRawAltitude - rawAltitudeM) > LANDING_ALTITUDE_CHANGE_THRESHOLD_M;
    if (filteredChanged) {
        m_landingDetectionReferenceAltitude = state1D.altitudeM;
    }
    if (rawChanged) {
        m_landingDetectionReferenceRawAltitude = rawAltitudeM;
    }
    const bool aboveAltitudeChangedThreshold = filteredChanged || rawChanged;
    if (m_landingDebounce.check(!aboveAltitudeChangedThreshold, timestamp.runtime_ms)) {
        return true;
    }
    return false;
}

void FlightStateDeterminer::resetUnknownStateDetection() {
    m_unknownStateTimer.reset();
    m_unknownStateHighTimer.reset();
    m_unknownStateVelocityLowPass.reset();
    m_unknownStateMaximumAltitude = -999999999.0f;
    m_unknownStateMinimumAltitude = 99999999.0f;
}

FlightState_e FlightStateDeterminer::getFlightState() const {
    return FlightState_e(m_flightState.get());
}


void FlightStateDeterminer::setFlightState(const Timestamp_s& timestamp, const FlightState_e& flightState) {
    m_stateTransitionTick = true;
    m_flightState.set(flightState);
    m_stateStopWatch.startWatch(timestamp.runtime_ms);
    if (flightState == ASCENT) {
        m_firstStageBurnout.set(false);
    }
}

const StopWatch* FlightStateDeterminer::getStateTimer() const {
    return &m_stateStopWatch;
}

const StopWatch* FlightStateDeterminer::getBurnoutTimer() const {
    return &m_burnoutStopWatch;
}

bool FlightStateDeterminer::isStateTransitionTick() const {
    return m_stateTransitionTick;
}

bool FlightStateDeterminer::getFirstStageBurnout() const {
    return m_firstStageBurnout.get();
}

float FlightStateDeterminer::getMaxAltitude() const {
    return m_maxAltitude;
}

float FlightStateDeterminer::getMaxVelocity() const {
    return m_maxVelocity;
}

float FlightStateDeterminer::getMaxAcceleration() const {
    return m_maxAcceleration;
}
