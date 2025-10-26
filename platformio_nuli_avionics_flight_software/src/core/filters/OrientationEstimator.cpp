#include "OrientationEstimator.h"

constexpr ConfigurationID_t OrientationEstimator::REQUIRED_CONFIGS[];

void OrientationEstimator::setup(HardwareAbstraction* hardware, Configuration* configuration) {
    m_hardware = hardware;
    m_configuration = configuration;
    m_debug = hardware->getDebugStream();

    m_gyroscopeBias = configuration->getConfigurable<GYROSCOPE_BIAS_c>();
    m_launchAngle = configuration->getConfigurable<LAUNCH_ANGLE_c>();

    // Initialize the base conditions immediately
    m_launchAngleAlarm.startAlarm(0, 0);
    m_gyroscopeBiasAlarm.startAlarm(0, 0);
}

const Orientation_s& OrientationEstimator::update(const Timestamp_s& timestamp, const FlightState_e& flightState) {
    if (flightState == PRE_FLIGHT) {
        updateLaunchAngle(timestamp);
        updateGyroscopeBias(timestamp);
    } else {
        // Assume we have one gyro for now, @todo handle full scale switching
        const Vector3D_s velocitiesRadS_board = m_hardware->getGyroscope(0)->getVelocitiesRadS_board_biasRemoved();
        integrateGyroscope(timestamp, velocitiesRadS_board);
        m_currentOrientation.angularVelocity = velocitiesRadS_board;
        m_currentOrientation.angle = QuaternionHelper::toEulerRPY(m_currentOrientation.angleQuaternion);
    }
    return getOrientation();
}

const Orientation_s& OrientationEstimator::getOrientation() const {
    return m_currentOrientation;
}

void OrientationEstimator::integrateGyroscope(const Timestamp_s& timestamp, const Vector3D_s& angularVelocity) {
    const float dt = float(timestamp.dt_ms) / 1000.0f;
    const Quaternion_s omega{angularVelocity.x, angularVelocity.y, angularVelocity.z, 0.0f};
    Quaternion_s q_dot = QuaternionHelper::multiply(m_currentOrientation.angleQuaternion, omega);
    q_dot.x *= 0.5f * dt;
    q_dot.y *= 0.5f * dt;
    q_dot.z *= 0.5f * dt;
    q_dot.w *= 0.5f * dt;
    m_currentOrientation.angleQuaternion.x += q_dot.x;
    m_currentOrientation.angleQuaternion.y += q_dot.y;
    m_currentOrientation.angleQuaternion.z += q_dot.z;
    m_currentOrientation.angleQuaternion.w += q_dot.w;
    QuaternionHelper::normalize(m_currentOrientation.angleQuaternion);
}

void OrientationEstimator::updateLaunchAngle(const Timestamp_s& timestamp) {
    if (m_hardware->getNumAccelerometers() > 0) {
        // Get acceleration and filter
        const Vector3D_s accelerationsMSS_board = m_hardware->getAccelerometer(0)->getAccelerationsMSS_board();
        m_launchAngleLowPassX.update(accelerationsMSS_board.x);
        m_launchAngleLowPassY.update(accelerationsMSS_board.y);
        m_launchAngleLowPassZ.update(accelerationsMSS_board.z);
        // Calculate orientation
        const Quaternion_s groundOrientation = QuaternionHelper::fromGravityVector({m_launchAngleLowPassX.value(), m_launchAngleLowPassY.value(), m_launchAngleLowPassZ.value()});
        // Determine if the value has changed enough, and if so update the reference
        const float deltaAngle = QuaternionHelper::angleBetween(groundOrientation, m_currentOrientation.angleQuaternion);
        if (m_launchAngleAlarm.isAlarmFinished(timestamp.runtime_ms)) {
            m_launchAngleAlarm.startAlarm(timestamp.runtime_ms, 1000);
            if (deltaAngle > 1.0f) {
                m_currentOrientation.angleQuaternion = groundOrientation;
                m_launchAngle.set(groundOrientation);
            }
        }
    } else {
        m_currentOrientation.angleQuaternion = QuaternionHelper::identity();
    }
}

void OrientationEstimator::updateGyroscopeBias(const Timestamp_s& timestamp) {
    GyroscopeBias_s currentGyroBias = m_gyroscopeBias.get();
    // Only allow updates to the configuration at max 1Hz
    const bool validUpdateTick = m_gyroscopeBiasAlarm.isAlarmFinished(timestamp.runtime_ms);
    if (validUpdateTick) m_gyroscopeBiasAlarm.startAlarm(timestamp.runtime_ms, 1000);
    // Loop through all gyros and update their biases
    bool isUpdated = false;
    for (uint32_t i = 0; i < m_hardware->getNumGyroscopes(); i++) {
        Gyroscope *gyroscope = m_hardware->getGyroscope(0);
        const Vector3D_s angularVelocitiesRadSRaw_sensor = gyroscope->getVelocitiesRadS_raw();
        m_lowPass[i].x.update(angularVelocitiesRadSRaw_sensor.x);
        m_lowPass[i].y.update(angularVelocitiesRadSRaw_sensor.y);
        m_lowPass[i].z.update(angularVelocitiesRadSRaw_sensor.z);
        if (validUpdateTick) {
            const Vector3D_s gyroscopeBiasFiltered = {m_lowPass[i].x.value(), m_lowPass[i].y.value(), m_lowPass[i].z.value()};
            const float deltaMagnitude = std::sqrt(
                (gyroscopeBiasFiltered.x - currentGyroBias.bias[i].x) * (gyroscopeBiasFiltered.x - currentGyroBias.bias[i].x) +
                (gyroscopeBiasFiltered.y - currentGyroBias.bias[i].y) * (gyroscopeBiasFiltered.y - currentGyroBias.bias[i].y) +
                (gyroscopeBiasFiltered.z - currentGyroBias.bias[i].z) * (gyroscopeBiasFiltered.z - currentGyroBias.bias[i].z));
            if (deltaMagnitude > 0.1) {
                currentGyroBias.bias[i] = gyroscopeBiasFiltered;
                gyroscope->setBiasOffset(gyroscopeBiasFiltered);
                isUpdated = true;
            }
        }
    }
    if (isUpdated) {
        m_gyroscopeBias.set(currentGyroBias);
    }
}
