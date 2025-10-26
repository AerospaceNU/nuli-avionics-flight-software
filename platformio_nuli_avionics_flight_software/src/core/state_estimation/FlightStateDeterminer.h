#ifndef FLIGHT_STATEMACHINE_H
#define FLIGHT_STATEMACHINE_H

#include "Avionics.h"
#include "../configuration/Configuration.h"
#include "core/filters/LowPass.h"
#include "util/Timer.h"

class FlightStateDeterminer {
public:
    constexpr static ConfigurationID_t REQUIRED_CONFIGS[] = {FLIGHT_STATE_c, GROUND_ELEVATION_c};

    void setup(Configuration* configuration);

    FlightState_e update(const Timestamp_s& timestamp, const State1D_s& state1D);

    FlightState_e getFlightState() const;

    void setFlightState(const Timestamp_s& timestamp, const FlightState_e& flightState);

    const StopWatch* getStateTimer() const;

private:
    bool hasLaunched(const Timestamp_s& timestamp, const State1D_s& state1D);

    bool apogeeReached(const Timestamp_s& timestamp, const State1D_s& state1D);

    bool hasLanded(const Timestamp_s& timestamp, const State1D_s& state1D);

    ConfigurationData<int32_t> m_flightState;
    Configuration* m_configuration = nullptr;

    float m_landingDetectionReferenceAltitude = 0;
    float m_maxAltitude = -999999999.0f;

    StopWatch m_stateStopWatch;

    Debounce m_launchDebounce = Debounce(LAUNCH_DEBOUNCE_TIMER_MS);
    Debounce m_apogeeDebounce = Debounce(APOGEE_DEBOUNCE_TIMER_MS);
    Debounce m_landingDebounce = Debounce(LANDING_DEBOUNCE_TIMER_MS);

    Debounce m_unknownStateTimer = Debounce(UNKNOWN_STATE_TIMER_MS);
    LowPass m_unknownStateVelocityLowPass{0.1};
    float m_unknownStateMaximumAltitude = -999999999.0f;
    float m_unknownStateMinimumAltitude = 99999999.0f;
};

#endif //FLIGHT_STATEMACHINE_H
