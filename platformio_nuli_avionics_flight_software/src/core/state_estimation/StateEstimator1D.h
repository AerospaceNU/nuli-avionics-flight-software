#ifndef STATEESTIMATER_H
#define STATEESTIMATER_H

#include "../filters/KalmanFilter1D.h"
#include "Avionics.h"
#include "core/HardwareAbstraction.h"
#include "../configuration/Configuration.h"
#include "../filters/LowPass.h"
#include "util/Timer.h"


class StateEstimator1D {
public:
    constexpr static ConfigurationID_t REQUIRED_CONFIGS[] = {BOARD_ORIENTATION_c, GROUND_ELEVATION_c, GROUND_TEMPERATURE_c};

    void setup(HardwareAbstraction* hardware, Configuration* configuration);

    State1D_s update(const Timestamp_s& timestamp, const FlightState_e &flightState);

    State1D_s getState1D() const;

    void reset();

private:
    float getPressurePa();

    float getAccelerationMSS(const FlightState_e &flightState) const;

    void updateGroundElevationReference(float unfilteredAltitudeM, const Timestamp_s& timestamp);

    void updateGroundTemperatureReference(float unfilteredAltitudeM, const Timestamp_s& timestamp);


    void updateBoardOrientationReference(const Timestamp_s &timestamp);

    State1D_s m_currentState1D = {};
    HardwareAbstraction* m_hardware = nullptr;
    Configuration* m_configuration = nullptr;

    ConfigurationData<float> m_groundElevation;
    ConfigurationData<float> m_groundTemperature;
    ConfigurationData<int32_t> m_boardOrientation;

    KalmanFilter1D m_kalmanFilter;

    float m_lastPressure = Constants::ATMOSPHERIC_PRESSURE_PA;

    LowPass m_groundTemperatureReferenceLowPass{0.01};

    bool m_reInitializeKalman = true;
    bool m_needNewGroundReference = true;
    LowPass m_groundElevationReferenceLowPass{0.01};
    Alarm m_groundElevationReferenceTimer;

    LowPass m_lowPassAX{0.01};
    LowPass m_lowPassAY{0.01};
    LowPass m_lowPassAZ{0.01};
    Alarm m_boardOrientationReferenceTimer;

};


#endif //STATEESTIMATER_H
