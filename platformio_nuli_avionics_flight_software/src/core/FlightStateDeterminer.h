#ifndef FLIGHT_STATEMACHINE_H
#define FLIGHT_STATEMACHINE_H

#include "Avionics.h"
#include "Configuration.h"
#include "core/filters/LowPass.h"

class FlightStateDeterminer {
public:
    constexpr static ConfigurationID_t REQUIRED_CONFIGS[] = {FLIGHT_STATE_c, GROUND_ELEVATION_c};

    void setup(Configuration* configuration);

    FlightState_e loopOnce(const State1D_s& state1D, const Timestamp_s& timestamp);

    FlightState_e getState() const;

    void setState(const FlightState_e& state, const Timestamp_s& timestamp);

    uint32_t getStateStartTime() const;

private:
    bool hasLaunched(const State1D_s& state1D, const Timestamp_s& timestamp);

    bool apogeeReached(const State1D_s& state1D, const Timestamp_s& timestamp);

    bool hasLanded(const State1D_s& state1D, const Timestamp_s& timestamp);

    ConfigurationData<int32_t> m_flightState;
    ConfigurationData<float> m_groundElevation;
    Configuration* m_configuration = nullptr;

    LowPass m_lowPass{0.01};

    uint32_t m_internalStateTransitionTimer = 0;
    uint32_t m_internalSecondaryTimer = 0;
    float m_landingDetectionReferenceAltitude = 0;

    float m_maxAltitude = 0;

    uint32_t m_flightStateStartTime = 0;
};

#endif //FLIGHT_STATEMACHINE_H
