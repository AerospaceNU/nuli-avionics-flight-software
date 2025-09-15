#include "PoseEstimator.h"

#include "../../ConstantsUnits.h"
#include "core/generic_hardware/Accelerometer.h"
#include "core/generic_hardware/Barometer.h"

constexpr ConfigurationID_e PoseEstimator::REQUIRED_CONFIGS[];

void PoseEstimator::setup(HardwareAbstraction* hardware, Configuration* configuration) {
    m_hardware = hardware;
    m_configuration = configuration;
    m_state = m_configuration->getConfigurable<STATE>();
}

Pose_s PoseEstimator::loopOnce() {
    // Start by getting all sensor measurements in their local frames, and combining redundant sensors
    // float pressurePa = getPressurePa();
    // Vector3D_s accelerationMSS = getAccelerationMSS();

    // Transform to global frame


    m_currentPose.timestamp_ms = m_hardware->getLoopTimestampMs();
    return m_currentPose;
}

Pose_s PoseEstimator::getPose() const {
    return m_currentPose;
}

float PoseEstimator::getPressurePa() const {
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

Vector3D_s PoseEstimator::getAccelerationMSS() const {
    if (m_state.get() == DESCENT) {
        return {0, 0, 0};
    }

    // for (int32_t i = 0; i < m_hardware->getNumAccelerometers(); i++) {
    //     Barometer* barometer = m_hardware->getBarometer(i);
    // }


    return {};
}

float PoseEstimator::calculateAltitudeM(const float pressurePa) {
    return (286.0 / Constants::LAPSE_RATE_K_M) *
        (pow(pressurePa / Constants::ATMOSPHERIC_PRESSURE_PA, -Constants::GAS_CONSTANT_J_KG_K * Constants::LAPSE_RATE_K_M / Constants::G_EARTH_MSS) - 1);
}
