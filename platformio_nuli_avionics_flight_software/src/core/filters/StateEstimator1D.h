#ifndef STATEESTIMATER_H
#define STATEESTIMATER_H

#include "Avionics.h"
#include "core/HardwareAbstraction.h"
#include "core/Configuration.h"
#include "altitude_kf.h"
#include "LowPass.h"


class StateEstimator1D {
public:
    constexpr static ConfigurationID_t REQUIRED_CONFIGS[] = {FLIGHT_STATE_c, GROUND_ELEVATION_c, GROUND_TEMPERATURE_c};

    void setup(HardwareAbstraction* hardware, Configuration* configuration);

    State1D_s loopOnce(const Timestamp_s& timestamp, const FlightState_e &flightState);

    State1D_s getState1D() const;

    void reset();

private:
    float getPressurePa();

    float getAccelerationMSS(const FlightState_e &flightState) const;

    void updateGroundReference(float unfilteredAltitudeM, const Timestamp_s& timestamp);


    State1D_s m_currentState1D = {};
    HardwareAbstraction* m_hardware = nullptr;
    Configuration* m_configuration = nullptr;

    ConfigurationData<float> m_groundElevation;
    ConfigurationData<float> m_groundTemperature;

    AltitudeKf m_kalmanFilter;

    float m_lastPressure = Constants::ATMOSPHERIC_PRESSURE_PA;


    bool m_reInitializeKalman = true;
    bool m_needNewGroundReference = false;
    LowPass m_lowPass{0.01};
    uint32_t m_groundReferenceTimer = 0;

};


#endif //STATEESTIMATER_H
