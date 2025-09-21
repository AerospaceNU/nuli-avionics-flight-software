#ifndef STATEESTIMATER_H
#define STATEESTIMATER_H

#include "Avionics.h"
#include "core/HardwareAbstraction.h"
#include "core/Configuration.h"


class StateEstimator1D {
public:
    constexpr static ConfigurationID_e REQUIRED_CONFIGS[] = {FLIGHT_STATE};


    void setup(HardwareAbstraction* hardware, Configuration* configuration);

    State1D_s loopOnce(const Timestamp_s& timestamp);

    State1D_s getState1D() const;

private:
    float getPressurePa() const;

    Vector3D_s getAccelerationMSS() const;

    static float calculateAltitudeM(float pressurePa);

    State1D_s m_currentState1D = {};
    HardwareAbstraction* m_hardware = nullptr;
    Configuration* m_configuration = nullptr;

    ConfigurationData<int32_t> m_flightState;
};


#endif //STATEESTIMATER_H
