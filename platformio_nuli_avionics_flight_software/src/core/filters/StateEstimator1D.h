#ifndef STATEESTIMATER_H
#define STATEESTIMATER_H

#include "Avionics.h"
#include "core/HardwareAbstraction.h"
#include "core/Configuration.h"
#include "altitude_kf.h"


class StateEstimator1D {
public:
    constexpr static ConfigurationID_t REQUIRED_CONFIGS[] = {FLIGHT_STATE_c, GROUND_ELEVATION_c, GROUND_TEMPERATURE_c};

    void setup(HardwareAbstraction* hardware, Configuration* configuration);

    State1D_s loopOnce(const Timestamp_s& timestamp);

    State1D_s getState1D() const;

private:
    float getPressurePa();

    float getAccelerationMSS() const;

    State1D_s m_currentState1D = {};
    HardwareAbstraction* m_hardware = nullptr;
    Configuration* m_configuration = nullptr;

    ConfigurationData<int32_t> m_flightState;
    ConfigurationData<float> m_groundElevation;
    ConfigurationData<float> m_groundTemperature;

    AltitudeKf kalmanFilter;

    float m_lastPressure = Constants::ATMOSPHERIC_PRESSURE_PA;
};


#endif //STATEESTIMATER_H
