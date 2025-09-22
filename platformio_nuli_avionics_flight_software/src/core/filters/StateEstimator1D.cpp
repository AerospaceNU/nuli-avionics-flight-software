#include "StateEstimator1D.h"

#include "../../ConstantsUnits.h"
#include "core/generic_hardware/Accelerometer.h"
#include "core/generic_hardware/Barometer.h"

constexpr ConfigurationID_e StateEstimator1D::REQUIRED_CONFIGS[];

void StateEstimator1D::setup(HardwareAbstraction* hardware, Configuration* configuration) {
    m_hardware = hardware;
    m_configuration = configuration;
    m_flightState = m_configuration->getConfigurable<FLIGHT_STATE>();
    m_groundElevation = m_configuration->getConfigurable<GROUND_ELEVATION>();


    kalmanFilter.setDeltaTime(float(m_hardware->getTargetLoopTimeMs()) / 1000.0f);
    kalmanFilter.setAccelerometerCovariance(1);
}

State1D_s StateEstimator1D::loopOnce(const Timestamp_s& timestamp) {
    // Start by getting all sensor measurements in their local frames, and combining redundant sensors
    const float pressurePa = getPressurePa();
    const float altitudeRawM = Barometer::calculateAltitudeM(pressurePa);
    const float accelerationMSS = getAccelerationMSS();

    // @todo update covariances with velocity

    kalmanFilter.predict();
    kalmanFilter.altitudeAndAccelerationDataUpdate(altitudeRawM - m_groundElevation.get(), accelerationMSS);

    m_currentState1D.altitudeM = kalmanFilter.getAltitude();
    m_currentState1D.velocityMS = kalmanFilter.getVelocity();
    m_currentState1D.accelerationMSS = kalmanFilter.getAcceleration();
    m_currentState1D.unfilteredNoOffsetAltitudeM = altitudeRawM;

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

float StateEstimator1D::getAccelerationMSS() const {
    if (m_flightState.get() == DESCENT) {
        return 0;
    }

    // @todo, full scale switching, bad reading detection, and coordinate transforms
    if (m_hardware->getNumAccelerometers() > 0) {
        return -m_hardware->getAccelerometer(0)->getAccelerationsMSS().x - float(Constants::G_EARTH_MSS);
    }

    return 0;
}
