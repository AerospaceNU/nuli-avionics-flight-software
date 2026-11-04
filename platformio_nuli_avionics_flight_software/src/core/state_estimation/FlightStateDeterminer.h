#ifndef FLIGHT_STATEMACHINE_H
#define FLIGHT_STATEMACHINE_H

#include "Avionics.h"
#include "core/configuration/Configuration.h"

#include "core/filters/LowPass.h"
#include "util/Timer.h"

class FlightStateDeterminer {
public:
    constexpr static ConfigurationID_t REQUIRED_CONFIGS[] = {FLIGHT_STATE_c, GROUND_ELEVATION_c, FIRST_STAGE_BURNOUT_c};

    void setup(Configuration* configuration);

    FlightState_e update(const Timestamp_s& timestamp, const State1D_s& state1D);

    FlightState_e getFlightState() const;

    void setFlightState(const Timestamp_s& timestamp, const FlightState_e& flightState);

    bool getFirstStageBurnout() const;

    const StopWatch* getStateTimer() const;

    const StopWatch* getBurnoutTimer() const;

    bool isStateTransitionTick() const;

    float getMaxAltitude() const;

    float getMaxVelocity() const;

    float getMaxAcceleration() const;

private:
    bool hasLaunched(const Timestamp_s& timestamp, const State1D_s& state1D);

    bool hasBurnedOut(const Timestamp_s& timestamp, const State1D_s& state1D);

    bool apogeeReached(const Timestamp_s& timestamp, const State1D_s& state1D);

    bool hasLanded(const Timestamp_s& timestamp, const State1D_s& state1D);

    /** @brief After a reboot mid-flight: the state the UNKNOWN window settles on, or UNKNOWN_FLIGHT_STATE while it is still deciding. */
    FlightState_e resolveUnknownState(const Timestamp_s& timestamp, const State1D_s& state1D);

    bool unknownStateLooksSubsonic() const;

    void resetUnknownStateDetection();

    Configuration* m_configuration = nullptr;

    ConfigurationData<int32_t> m_flightState;
    bool m_unknownFlightState = false; ///< UNKNOWN is never stored: FRAM keeps ASCENT/DESCENT until it resolves
    bool m_stateTransitionTick = false;
    StopWatch m_stateStopWatch;

    float m_maxAltitude = -999999999.0f;
    float m_maxVelocity = -999999999.0f;
    float m_maxAcceleration = -999999999.0f;

    Debounce m_launchDebounce = Debounce(LAUNCH_DEBOUNCE_TIMER_MS);

    ConfigurationData<uint32_t> m_firstStageBurnout;
    Debounce m_burnoutDebounce = Debounce(BURNOUT_DEBOUNCE_TIMER_MS);
    StopWatch m_burnoutStopWatch;

    Debounce m_machLockDebounce = Debounce(MACH_LOCKOUT_TIMER_MS);
    Debounce m_apogeeDebounce = Debounce(APOGEE_DEBOUNCE_TIMER_MS);
    bool m_barometerValidInFlight = false; ///< apogee needs a barometer that worked at some point since launch

    Debounce m_landingDebounce = Debounce(LANDING_DEBOUNCE_TIMER_MS);
    float m_landingDetectionReferenceAltitude = 0;
    float m_landingDetectionReferenceRawAltitude = 0;
    LowPass m_landingRawAltitudeLowPass{0.1};

    Alarm m_unknownStateWindowAlarm;
    Alarm m_unknownStateHighAlarm; ///< started on entering UNKNOWN
    LowPass m_unknownStateVelocityLowPass{0.1};
    float m_unknownStateMaximumAltitude = -999999999.0f;
    float m_unknownStateMinimumAltitude = 99999999.0f;
    double m_unknownStateFitN = 0;
    double m_unknownStateFitT = 0;
    double m_unknownStateFitH = 0;
    double m_unknownStateFitTT = 0;
    double m_unknownStateFitTH = 0;
    double m_unknownStateFitHH = 0;
    double m_unknownStateFitStartS = 0;
    double m_unknownStateFitEndS = 0;
    float m_unknownStateAccelerometerSum = 0;
    uint32_t m_unknownStateAccelerometerCount = 0;
    float m_unknownStateAccelerometerMinimum = 999999999.0f;
    float m_unknownStateAccelerometerMaximum = -999999999.0f;
    bool m_unknownStateAccelerometerInvalid = false;
};

#endif //FLIGHT_STATEMACHINE_H
