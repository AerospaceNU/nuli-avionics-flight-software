#include "FlightStateDeterminer.h"
#include <algorithm>

void FlightStateDeterminer::setup(Configuration* configuration) {
    m_configuration = configuration;
    m_flightState = m_configuration->getConfigurable<FLIGHT_STATE_c>();
    m_firstStageBurnout = m_configuration->getConfigurable<FIRST_STAGE_BURNOUT_c>();
    m_firstStageBurnout.set(false);
    m_machLockDebounce.setAlreadyMet();

    // We need to determine the initial "boot" state
    // If a flight was completed, we can assume that we are not currently in the air
    if (getFlightState() == POST_FLIGHT) {
        setFlightState({0, 0, 0}, PRE_FLIGHT);
    }
    // If we have any other state that's not PRE_FLIGHT or POST_FLIGHT, we can no longer be sure that we are in that state.
    if (getFlightState() == ASCENT || getFlightState() == DESCENT || getFlightState() == UNKNOWN_FLIGHT_STATE) {
        setFlightState({0, 0, 0}, UNKNOWN_FLIGHT_STATE);
    }
}

FlightState_e FlightStateDeterminer::update(const Timestamp_s& timestamp, const State1D_s& state1D) {
    m_stateTransitionTick = false;
    if (getFlightState() == PRE_FLIGHT) {
        if (hasLaunched(timestamp, state1D)) {
            setFlightState(timestamp, ASCENT);
        }
    } else if (getFlightState() == ASCENT) {
        m_maxVelocity = std::max(m_maxVelocity, state1D.velocityMS);
        m_maxAcceleration = std::max(m_maxAcceleration, state1D.accelerationMSS);
        if (state1D.barometerValid) m_barometerValidInFlight = true;
        // Check for burnout
        if (hasBurnedOut(timestamp, state1D) && !m_firstStageBurnout.get()) {
            m_firstStageBurnout.set(true);
            m_burnoutStopWatch.startWatch(timestamp.runtime_ms);
        }
        // Check for apogee
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
        const FlightState_e resolvedState = resolveUnknownState(timestamp, state1D);
        if (resolvedState != UNKNOWN_FLIGHT_STATE) {
            setFlightState(timestamp, resolvedState);
            if (resolvedState == ASCENT && !unknownStateLooksSubsonic()) m_machLockDebounce.restartTimingFrom(timestamp.runtime_ms);
            resetUnknownStateDetection();
        }
    } else {
        setFlightState(timestamp, UNKNOWN_FLIGHT_STATE);
    }
    return getFlightState();
}

FlightState_e FlightStateDeterminer::resolveUnknownState(const Timestamp_s& timestamp, const State1D_s& state1D) {
    if (!state1D.barometerValid && !m_unknownStateHighAlarm.isAlarmFinished(timestamp.runtime_ms)) return UNKNOWN_FLIGHT_STATE;
    if (!m_unknownStateWindowAlarm.isInitialized()) m_unknownStateWindowAlarm.startAlarm(timestamp.runtime_ms, UNKNOWN_STATE_TIMER_MS);
    const bool inWindow = !m_unknownStateWindowAlarm.isAlarmFinished(timestamp.runtime_ms);
    if (state1D.barometerValid) {
        m_unknownStateVelocityLowPass.update(state1D.velocityMS);
        m_unknownStateMaximumAltitude = std::max(m_unknownStateMaximumAltitude, state1D.altitudeM);
        m_unknownStateMinimumAltitude = std::min(m_unknownStateMinimumAltitude, state1D.altitudeM);
    }
    if (inWindow && state1D.barometerValid) {
        const double t = m_unknownStateWindowAlarm.getTimeElapsed(timestamp.runtime_ms) / 1000.0;
        const double h = state1D.unfilteredNoOffsetAltitudeM;
        if (m_unknownStateFitN == 0) m_unknownStateFitStartS = t;
        m_unknownStateFitEndS = t;
        m_unknownStateFitN += 1;
        m_unknownStateFitT += t;
        m_unknownStateFitH += h;
        m_unknownStateFitTT += t * t;
        m_unknownStateFitTH += t * h;
        m_unknownStateFitHH += h * h;
    }
    if (inWindow) {
        m_unknownStateAccelerometerSum += state1D.accelerometerMagnitudeMSS;
        m_unknownStateAccelerometerCount++;
        m_unknownStateAccelerometerMinimum = std::min(m_unknownStateAccelerometerMinimum, state1D.accelerometerMagnitudeMSS);
        m_unknownStateAccelerometerMaximum = std::max(m_unknownStateAccelerometerMaximum, state1D.accelerometerMagnitudeMSS);
        if (state1D.accelerometerMagnitudeMSS < 0) m_unknownStateAccelerometerInvalid = true;
        return UNKNOWN_FLIGHT_STATE;
    }

    const bool sufficientDeltaAltitude = m_unknownStateMaximumAltitude - m_unknownStateMinimumAltitude > UNKNOWN_STATE_ALTITUDE_CHANGE_THRESHOLD_M;
    const bool sufficientVelocity = std::fabs(m_unknownStateVelocityLowPass.value()) > UNKNOWN_STATE_VELOCITY_THRESHOLD_MS;
    const bool highAboveGround = m_unknownStateMaximumAltitude > LAUNCH_ALTITUDE_THRESHOLD_M;
    if (sufficientDeltaAltitude && sufficientVelocity && m_unknownStateVelocityLowPass.value() < 0 && m_flightState.get() == DESCENT) return DESCENT;
    if (sufficientDeltaAltitude && sufficientVelocity) return ASCENT; // Always set to ascent because ASCENT will rapidly determine if in ASCENT or DESCENT
    if (highAboveGround && !m_unknownStateHighAlarm.isAlarmFinished(timestamp.runtime_ms)) return UNKNOWN_FLIGHT_STATE;
    return PRE_FLIGHT;
}

bool FlightStateDeterminer::hasLaunched(const Timestamp_s& timestamp, const State1D_s& state1D) {
    // We have launched if we have seen a sufficiently high acceleration for enough time
    const bool aboveLaunchAcceleration = state1D.accelerationMSS >= LAUNCH_ACCELERATION_THRESHOLD_MSS;
    // We have also launched if we have reached a high enough altitude
    // Note that the ground reference recalculation creates a race condition here:
    // Pose is measured relative to the ground, and the ground reference is updated in PRE_FLIGHT
    // This means that if the ground reference was updated every tick to be the current altitude, this would never trigger.
    // A low pass filter with a low constant is used in StateEstimator1D to allow the race condition to work out.
    const bool aboveLaunchAltitude = state1D.barometerValid && state1D.altitudeM >= LAUNCH_ALTITUDE_THRESHOLD_M;
    // Run the debounce check
    return m_launchDebounce.check(aboveLaunchAcceleration || aboveLaunchAltitude, timestamp.runtime_ms);
}

bool FlightStateDeterminer::hasBurnedOut(const Timestamp_s& timestamp, const State1D_s& state1D) {
    return m_burnoutDebounce.check(state1D.accelerationMSS < 0, timestamp.runtime_ms);
}

bool FlightStateDeterminer::apogeeReached(const Timestamp_s& timestamp, const State1D_s& state1D) {
    // Mach lockout
    if (m_machLockDebounce.check(state1D.velocityMS < MACH_LOCKOUT_THRESHOLD_MS, timestamp.runtime_ms)) {
        // Track maximum altitude
        m_maxAltitude = std::max(m_maxAltitude, state1D.altitudeM);
        // Run debounce checks
        const bool goingDown = state1D.velocityMS < 0;
        const bool belowApogeeChangeThreshold = state1D.altitudeM < m_maxAltitude - APOGEE_ALTITUDE_CHANGE_THRESHOLD_M;
        return m_apogeeDebounce.check(m_barometerValidInFlight && goingDown && belowApogeeChangeThreshold, timestamp.runtime_ms);
    }
    m_apogeeDebounce.reset();
    return false;
}

bool FlightStateDeterminer::hasLanded(const Timestamp_s& timestamp, const State1D_s& state1D) {
    if (!state1D.barometerValid) return false;
    const float rawAltitudeM = m_landingRawAltitudeLowPass.update(state1D.unfilteredNoOffsetAltitudeM);
    const bool filteredChanged = std::fabs(m_landingDetectionReferenceAltitude - state1D.altitudeM) > LANDING_ALTITUDE_CHANGE_THRESHOLD_M;
    const bool rawChanged = std::fabs(m_landingDetectionReferenceRawAltitude - rawAltitudeM) > LANDING_ALTITUDE_CHANGE_THRESHOLD_M;
    if (filteredChanged) m_landingDetectionReferenceAltitude = state1D.altitudeM;
    if (rawChanged) m_landingDetectionReferenceRawAltitude = rawAltitudeM;
    return m_landingDebounce.check(!filteredChanged && !rawChanged, timestamp.runtime_ms);
}

bool FlightStateDeterminer::unknownStateLooksSubsonic() const {
    if (m_unknownStateAccelerometerInvalid || m_unknownStateAccelerometerCount == 0) return false;
    if (m_unknownStateAccelerometerMaximum <= m_unknownStateAccelerometerMinimum) return false;
    if (m_unknownStateFitN < 3 || m_unknownStateFitN < UNKNOWN_STATE_SUBSONIC_MIN_VALID_FRACTION * m_unknownStateAccelerometerCount) return false;
    if (m_unknownStateFitEndS - m_unknownStateFitStartS < UNKNOWN_STATE_SUBSONIC_MIN_VALID_FRACTION * UNKNOWN_STATE_TIMER_MS / 1000.0) return false;
    if (m_unknownStateFitH / m_unknownStateFitN >= UNKNOWN_STATE_SUBSONIC_MAX_ALTITUDE_M) return false;
    const double ctt = m_unknownStateFitTT - m_unknownStateFitT * m_unknownStateFitT / m_unknownStateFitN;
    const double cth = m_unknownStateFitTH - m_unknownStateFitT * m_unknownStateFitH / m_unknownStateFitN;
    const double chh = m_unknownStateFitHH - m_unknownStateFitH * m_unknownStateFitH / m_unknownStateFitN;
    if (ctt <= 0) return false;
    const double slope = cth / ctt;
    const double residualVariance = (chh - cth * slope) / m_unknownStateFitN;
    const bool lowDrag = m_unknownStateAccelerometerSum / (float)m_unknownStateAccelerometerCount < UNKNOWN_STATE_SUBSONIC_ACCELERATION_MSS;
    const bool slow = std::fabs(slope) < UNKNOWN_STATE_SUBSONIC_VELOCITY_MS;
    const bool clean = residualVariance < UNKNOWN_STATE_SUBSONIC_RESIDUAL_M * UNKNOWN_STATE_SUBSONIC_RESIDUAL_M;
    return lowDrag && slow && clean;
}

void FlightStateDeterminer::resetUnknownStateDetection() {
    m_unknownStateWindowAlarm.reset();
    m_unknownStateVelocityLowPass.reset();
    m_unknownStateMaximumAltitude = -999999999.0f;
    m_unknownStateMinimumAltitude = 99999999.0f;
    m_unknownStateFitN = 0;
    m_unknownStateFitT = 0;
    m_unknownStateFitH = 0;
    m_unknownStateFitTT = 0;
    m_unknownStateFitTH = 0;
    m_unknownStateFitHH = 0;
    m_unknownStateFitStartS = 0;
    m_unknownStateFitEndS = 0;
    m_unknownStateAccelerometerSum = 0;
    m_unknownStateAccelerometerCount = 0;
    m_unknownStateAccelerometerMinimum = 999999999.0f;
    m_unknownStateAccelerometerMaximum = -999999999.0f;
    m_unknownStateAccelerometerInvalid = false;
}

FlightState_e FlightStateDeterminer::getFlightState() const {
    return m_unknownFlightState ? UNKNOWN_FLIGHT_STATE : FlightState_e(m_flightState.get());
}

void FlightStateDeterminer::setFlightState(const Timestamp_s& timestamp, const FlightState_e& flightState) {
    m_stateTransitionTick = true;
    m_unknownFlightState = flightState == UNKNOWN_FLIGHT_STATE;
    if (!m_unknownFlightState) m_flightState.set(flightState);
    m_stateStopWatch.startWatch(timestamp.runtime_ms);
    if (flightState == UNKNOWN_FLIGHT_STATE) {
        m_unknownStateHighAlarm.startAlarm(timestamp.runtime_ms, UNKNOWN_STATE_HIGH_TIMER_MS);
    }
    if (flightState == ASCENT) {
        m_barometerValidInFlight = false;
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
