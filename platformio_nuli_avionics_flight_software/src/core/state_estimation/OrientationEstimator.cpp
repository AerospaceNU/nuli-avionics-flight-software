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

    GyroscopeBias_s currentGyroBias = m_gyroscopeBias.get();
    for (uint32_t i = 0; i < m_hardware->getNumGyroscopes(); i++) {
        m_hardware->getGyroscope(i)->setBiasOffset(currentGyroBias.bias[i]);
    }
}

// Assume we have one gyro for now, @todo handle full scale switching
const Orientation_s& OrientationEstimator::update(const Timestamp_s& timestamp, const FlightState_e& flightState) {
    if (flightState == PRE_FLIGHT && timestamp.runtime_ms < 10 * 1000) {
        // updateGyroscopeBias(timestamp);
        m_currentOrientation.angularVelocity = m_hardware->getGyroscope(0)->getVelocitiesRadS_board_biasRemoved();
        m_currentOrientation.angleQuaternion = updateLaunchAngle(timestamp);
        computeTiltAndTwist(m_currentOrientation.angleQuaternion, m_currentOrientation.tilt);
    } else {
        Quaternion q = updateLaunchAngle(timestamp);
        float tilt;
        computeTiltAndTwist(q, tilt);

        const Vector3D_s velocitiesRadS_board = m_hardware->getGyroscope(0)->getVelocitiesRadS_board_biasRemoved();
        m_currentOrientation.angularVelocity = velocitiesRadS_board;
        m_currentOrientation.angleQuaternion = integrateGyroscope(timestamp, velocitiesRadS_board);
        computeTiltAndTwist(m_currentOrientation.angleQuaternion, m_currentOrientation.tilt);
        m_debug->message("%.2f\t%.2f\t%.2f", m_currentOrientation.tilt, tilt, m_currentOrientation.tilt - tilt);
        // m_debug->message("%.2f\t%.2f\t%.2f\t%.2f", m_currentOrientation.angleQuaternion.a, m_currentOrientation.angleQuaternion.b, m_currentOrientation.angleQuaternion.c, m_currentOrientation.angleQuaternion.d);

    }
    return getOrientation();
}

const Orientation_s& OrientationEstimator::getOrientation() const {
    return m_currentOrientation;
}

Quaternion OrientationEstimator::integrateGyroscope(const Timestamp_s& timestamp, const Vector3D_s& angularVelocity) const {
    const float dt = float(timestamp.dt_ms) / 1000.0f;

    // Angular velocity as a pure quaternion (w = 0)
    Quaternion omega(angularVelocity.x, angularVelocity.y, angularVelocity.z);

    // Current orientation quaternion
    Quaternion q = m_currentOrientation.angleQuaternion;

    // Quaternion derivative: q_dot = 0.5 * q * omega
    Quaternion q_dot = q;
    q_dot *= omega;
    q_dot *= 0.5f;

    // Integrate
    q.a += q_dot.a * dt;
    q.b += q_dot.b * dt;
    q.c += q_dot.c * dt;
    q.d += q_dot.d * dt;

    // Normalize
    q.normalize();

    // Store updated orientation
    return q;
}

Quaternion OrientationEstimator::updateLaunchAngle(const Timestamp_s& timestamp) const {
    // Get accelerometer reading in m/s^2
    Vector3D_s accel = m_hardware->getAccelerometer(0)->getAccelerationsMSS_board();

    // Normalize the accelerometer vector
    float mag = std::sqrt(accel.x * accel.x + accel.y * accel.y + accel.z * accel.z);
    if (mag < 1e-6f) return Quaternion(); // avoid division by zero
    accel.x /= mag;
    accel.y /= mag;
    accel.z /= mag;

    // Compute pitch and roll (in radians)
    // pitch = rotation around Y axis
    // roll  = rotation around X axis
    float pitch = std::asin(-accel.x);            // rotation about Y-axis
    float roll  = std::atan2(accel.y, accel.z);  // rotation about X-axis
    float yaw   = 0.0f;                           // cannot measure yaw with accelerometer

    // Create quaternion using your library
    Quaternion q = Quaternion::from_euler_rotation(roll, pitch, yaw);
    return q;
}


void OrientationEstimator::updateGyroscopeBias(const Timestamp_s& timestamp) {
    GyroscopeBias_s currentGyroBias = m_gyroscopeBias.get();
    // Only allow updates to the configuration at max 1Hz
    const bool validUpdateTick = m_gyroscopeBiasAlarm.isAlarmFinished(timestamp.runtime_ms);
    if (validUpdateTick) m_gyroscopeBiasAlarm.startAlarm(timestamp.runtime_ms, 1000);
    // Loop through all gyros and update their biases
    bool isUpdated = false;
    for (uint32_t i = 0; i < m_hardware->getNumGyroscopes(); i++) {
        Gyroscope* gyroscope = m_hardware->getGyroscope(i);
        const Vector3D_s angularVelocitiesRadSRaw_sensor = gyroscope->getVelocitiesRadS_raw();
        constexpr float thresh = 0.05;
        if (angularVelocitiesRadSRaw_sensor.x < thresh && angularVelocitiesRadSRaw_sensor.y < thresh && angularVelocitiesRadSRaw_sensor.z < thresh) {
            m_lowPass[i].x.update(angularVelocitiesRadSRaw_sensor.x);
            m_lowPass[i].y.update(angularVelocitiesRadSRaw_sensor.y);
            m_lowPass[i].z.update(angularVelocitiesRadSRaw_sensor.z);
        }
        if (validUpdateTick) {
            const Vector3D_s gyroscopeBiasFiltered = {m_lowPass[i].x.value(), m_lowPass[i].y.value(), m_lowPass[i].z.value()};
            const float deltaMagnitude = std::sqrt(
                (gyroscopeBiasFiltered.x - currentGyroBias.bias[i].x) * (gyroscopeBiasFiltered.x - currentGyroBias.bias[i].x) +
                (gyroscopeBiasFiltered.y - currentGyroBias.bias[i].y) * (gyroscopeBiasFiltered.y - currentGyroBias.bias[i].y) +
                (gyroscopeBiasFiltered.z - currentGyroBias.bias[i].z) * (gyroscopeBiasFiltered.z - currentGyroBias.bias[i].z));
            if (deltaMagnitude > 0.0001) {
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

void OrientationEstimator::computeTiltAndTwist(const Quaternion& q, float& tiltDeg) {
    // Make a copy (since q is const)
    Quaternion norm = q;
    norm.normalize();

    // Rotation matrix element for body Z axis in world frame
    const float r22 = 1 - 2 * (norm.b * norm.b + norm.c * norm.c);

    // Clamp to [-1, 1] to avoid NaNs from acos
    float zClamped = r22;
    if (zClamped > 1.0f) zClamped = 1.0f;
    else if (zClamped < -1.0f) zClamped = -1.0f;

    // Tilt = angle from vertical (deg)
    tiltDeg = std::acos(zClamped) * 180.0f / float(Constants::PI_VAL);
}
