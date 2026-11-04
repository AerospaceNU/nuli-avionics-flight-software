#ifndef STATEESTIMATER_H
#define STATEESTIMATER_H

#include "core/filters/KalmanFilter1D.h"
#include "Avionics.h"
#include "ConstantsUnits.h"
#include "core/HardwareAbstraction.h"
#include "core/configuration/Configuration.h"
#include "core/filters/LowPass.h"
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

    float getAccelerationMSS(const FlightState_e &flightState);

    void updateGroundElevationReference(float pressurePa, const Timestamp_s& timestamp);

    /** @brief Barometer covariance scale for transonic artifacts: 1, ramping to MACH_BAROMETER_COVARIANCE_SCALE between
     *         MACH_RAMP_START_MS and MACH_RAMP_END_MS of filter velocity; only while the accelerometer can carry the filter. */
    float machBarometerCovarianceScale();

    void updateGroundTemperatureReference(float unfilteredAltitudeM, const Timestamp_s& timestamp);


    void updateBoardOrientationReference(const Timestamp_s &timestamp);

    /** @brief The signed axis with the largest magnitude. */
    static AxisDirection dominantAxis(const Vector3D_s& vector);

    HardwareAbstraction* m_hardware = nullptr;
    Configuration* m_configuration = nullptr;

    ConfigurationData<float> m_groundElevation;
    ConfigurationData<float> m_groundTemperature;
    ConfigurationData<int32_t> m_boardOrientation;

    KalmanFilter1D m_kalmanFilter;
    bool m_reInitializeKalman = true;
    State1D_s m_currentState1D = {};

    float m_lastPressure = Constants::ATMOSPHERIC_PRESSURE_PA;
    bool m_barometerValid = false;
    bool m_barometerEverValid = false;
    uint32_t m_barometerInvalidTicks = 0;
    bool m_accelerationValid = false; ///< outside PRE_FLIGHT/ASCENT the acceleration is a 0 measurement, not the accelerometer

    bool m_needNewGroundReference = true;
    Alarm m_groundElevationReferenceTimer;
    LowPass m_groundPressureReferenceLowPass{0.01};
    LowPass m_groundTemperatureReferenceLowPass{0.01};

    Alarm m_boardOrientationReferenceTimer;
    LowPass m_lowPassAX{0.01};
    LowPass m_lowPassAY{0.01};
    LowPass m_lowPassAZ{0.01};

};


#endif //STATEESTIMATER_H
