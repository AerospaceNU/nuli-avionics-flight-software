#include "StateEstimator1D.h"

#include "../../ConstantsUnits.h"
#include "core/generic_hardware/Accelerometer.h"
#include "core/generic_hardware/Barometer.h"

constexpr ConfigurationID_e StateEstimator1D::REQUIRED_CONFIGS[];

void StateEstimator1D::setup(HardwareAbstraction* hardware, Configuration* configuration) {
    m_hardware = hardware;
    m_configuration = configuration;
    m_flightState = m_configuration->getConfigurable<FLIGHT_STATE>();
}

State1D_s StateEstimator1D::loopOnce(const Timestamp_s& timestamp) {
    // Start by getting all sensor measurements in their local frames, and combining redundant sensors
    const float pressurePa = getPressurePa();
    const Vector3D_s accelerationMSS = getAccelerationMSS();

    // Transform to global frame

    m_currentState1D.altitudeM = pressurePa;

    return m_currentState1D;
}

State1D_s StateEstimator1D::getState1D() const {
    return m_currentState1D;
}

float StateEstimator1D::getPressurePa() const {
    float pressurePa = 0;
    int32_t count = 0;
    for (int32_t i = 0; i < m_hardware->getNumBarometers(); i++) {
        Barometer* barometer = m_hardware->getBarometer(i);
        if (barometer->validReading()) {
            pressurePa += barometer->getPressurePa();
            count++;
        }
    }

    if (count == 0) {
        return -9999999.0; // @todo handle ticks with no valid sensor readings
    }

    return pressurePa / float(count);
}

Vector3D_s StateEstimator1D::getAccelerationMSS() const {
    if (m_flightState.get() == DESCENT) {
        return {0, 0, 0};
    }

    // for (int32_t i = 0; i < m_hardware->getNumAccelerometers(); i++) {
    //     Barometer* barometer = m_hardware->getBarometer(i);
    // }


    return {};
}

float StateEstimator1D::calculateAltitudeM(const float pressurePa) {
    return (286.0 / Constants::LAPSE_RATE_K_M) *
        (pow(pressurePa / Constants::ATMOSPHERIC_PRESSURE_PA, -Constants::GAS_CONSTANT_J_KG_K * Constants::LAPSE_RATE_K_M / Constants::G_EARTH_MSS) - 1);
}
