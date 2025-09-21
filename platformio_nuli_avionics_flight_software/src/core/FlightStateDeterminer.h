#ifndef FLIGHT_STATEMACHINE_H
#define FLIGHT_STATEMACHINE_H

#include "Avionics.h"
#include "Configuration.h"
#include "ConfigurationRegistry.h"

class FlightStateDeterminer {
public:
    constexpr static ConfigurationID_e REQUIRED_CONFIGS[] = {FLIGHT_STATE};

    void setup(Configuration* configuration);

    FlightState_e loopOnce(const State1D_s& state1D, const Timestamp_s& timestamp);

    FlightState_e getState() const;

    uint32_t getStateStartTime() const;

private:
    bool hasLaunched(const State1D_s& state1D, const Timestamp_s& timestamp);

    bool apogeeReached(const State1D_s& state1D, const Timestamp_s& timestamp);

    bool hasLanded(const State1D_s& state1D, const Timestamp_s& timestamp);

    ConfigurationData<int32_t> m_flightState;
    Configuration* m_configuration = nullptr;

    uint32_t m_internalStateTransitionTimer = 0;
    double m_landingDetectionReferenceAltitude = 0;

    uint32_t m_flightStateStartTime = 0;
};

#endif //FLIGHT_STATEMACHINE_H
