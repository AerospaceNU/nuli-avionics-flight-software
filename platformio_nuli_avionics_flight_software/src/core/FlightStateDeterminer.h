#ifndef FLIGHT_STATEMACHINE_H
#define FLIGHT_STATEMACHINE_H

#include "Avionics.h"
#include "Configuration.h"
#include "core/filters/LowPass.h"
#include "util/Debounce.h"

class FlightStateDeterminer {
public:
    constexpr static ConfigurationID_t REQUIRED_CONFIGS[] = {FLIGHT_STATE_c, GROUND_ELEVATION_c};

    void setup(Configuration* configuration);

    FlightState_e loopOnce(const Timestamp_s& timestamp, const State1D_s& state1D);

    FlightState_e getFlightState() const;

    void setFlightState(const Timestamp_s& timestamp, const FlightState_e& flightState);

    uint32_t getStateStartTime() const;

private:
    bool hasLaunched(const Timestamp_s& timestamp, const State1D_s& state1D);

    bool apogeeReached(const Timestamp_s& timestamp, const State1D_s& state1D);

    bool hasLanded(const Timestamp_s& timestamp, const State1D_s& state1D);

    ConfigurationData<int32_t> m_flightState;
    Configuration* m_configuration = nullptr;

    float m_landingDetectionReferenceAltitude = 0;
    float m_maxAltitude = 0;
    uint32_t m_flightStateStartTime = 0;

    Debounce launchDebounce = Debounce(LAUNCH_DEBOUNCE_TIMER_MS);
    Debounce apogeeDebounce = Debounce(APOGEE_DEBOUNCE_TIMER_MS);
    Debounce landingDebounce = Debounce(LANDING_DEBOUNCE_TIMER_MS);
};

#endif //FLIGHT_STATEMACHINE_H
