#include "StateEstimator1D.h"
#include "ConstantsUnits.h"
#include "core/generic_hardware/Accelerometer.h"
#include "core/generic_hardware/Barometer.h"

constexpr ConfigurationID_t StateEstimator1D::REQUIRED_CONFIGS[];

void StateEstimator1D::setup(HardwareAbstraction* hardware, Configuration* configuration) {
    m_hardware = hardware;
    m_configuration = configuration;
    m_groundElevation = m_configuration->getConfigurable<GROUND_ELEVATION_c>();
    m_groundTemperature = m_configuration->getConfigurable<GROUND_TEMPERATURE_c>();

    m_kalmanFilter.setDeltaTime(float(m_hardware->getTargetLoopTimeMs()) / 1000.0f);
    m_kalmanFilter.setAccelerometerCovariance(1);
}

State1D_s StateEstimator1D::loopOnce(const Timestamp_s& timestamp, const FlightState_e &flightState) {
    // Start by getting all sensor measurements in their local frames, and combining redundant sensors
    const float pressurePa = getPressurePa();
    const float altitudeRawM = Barometer::calculateAltitudeM(pressurePa, m_groundTemperature.get());
    const float accelerationMSS = getAccelerationMSS(flightState);

     if (flightState == PRE_FLIGHT) {
        updateGroundReference(altitudeRawM, timestamp);
    }

    if (m_reInitializeKalman) {
        m_kalmanFilter.restState(altitudeRawM - m_groundElevation.get(), 0, accelerationMSS);
        m_reInitializeKalman = false;
    }

    // @todo update covariances with velocity

    m_kalmanFilter.predict();
    m_kalmanFilter.altitudeAndAccelerationDataUpdate(altitudeRawM - m_groundElevation.get(), accelerationMSS);

    m_currentState1D.altitudeM = m_kalmanFilter.getAltitude();
    m_currentState1D.velocityMS = m_kalmanFilter.getVelocity();
    m_currentState1D.accelerationMSS = m_kalmanFilter.getAcceleration();
    m_currentState1D.unfilteredNoOffsetAltitudeM = altitudeRawM;

    return m_currentState1D;
}

State1D_s StateEstimator1D::getState1D() const {
    return m_currentState1D;
}

float StateEstimator1D::getPressurePa() {
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
        return m_lastPressure;
    }

    m_lastPressure = pressurePa / float(count);
    return m_lastPressure;
}

float StateEstimator1D::getAccelerationMSS(const FlightState_e &flightState) const {
    // @todo, full scale switching, bad reading detection, and coordinate transforms
    if ((flightState == PRE_FLIGHT || flightState == ASCENT) && m_hardware->getNumAccelerometers() > 0) {
        return -m_hardware->getAccelerometer(0)->getAccelerationsMSS().x - float(Constants::G_EARTH_MSS);
    }

    return 0;
}

void StateEstimator1D::reset() {
    m_reInitializeKalman = true;
    m_needNewGroundReference = true;
    m_lowPass.reset();
}

void StateEstimator1D::updateGroundReference(const float unfilteredAltitudeM, const Timestamp_s& timestamp) {
    m_lowPass.update(unfilteredAltitudeM);
    if (m_needNewGroundReference || timestamp.runtime_ms - m_groundReferenceTimer > 1000) {
        if (m_needNewGroundReference || abs(m_groundElevation.get() - m_lowPass.value()) > 2.0f) {
            m_needNewGroundReference = false;
            m_groundElevation.set(m_lowPass.value());
            m_hardware->getDebugStream()->message("Ground elevation set to %f", m_groundElevation.get());
        }
        m_groundReferenceTimer = timestamp.runtime_ms;
    }
}