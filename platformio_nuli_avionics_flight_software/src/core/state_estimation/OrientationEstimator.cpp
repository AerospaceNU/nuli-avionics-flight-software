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

    m_currentOrientation.angleQuaternion = m_launchAngle.get();

    // Ensure all gyros start compensated
    GyroscopeBias_s currentGyroBias = m_gyroscopeBias.get();
    for (uint32_t i = 0; i < m_hardware->getNumGyroscopes(); i++) {
        m_hardware->getGyroscope(i)->setBiasOffset(currentGyroBias.bias[i]);
        m_lowPass[i].x.forceSet(currentGyroBias.bias[i].x);
        m_lowPass[i].y.forceSet(currentGyroBias.bias[i].y);
        m_lowPass[i].z.forceSet(currentGyroBias.bias[i].z);
    }
}

// Assume we have one gyro for now, @todo handle full scale switching
const Orientation_s& OrientationEstimator::update(const Timestamp_s& timestamp, const FlightState_e& flightState) {
    if (flightState == PRE_FLIGHT && timestamp.runtime_ms < 10 * 1000) {
        updateGyroscopeBias(timestamp);
        m_currentOrientation.angularVelocity = m_hardware->getGyroscope(0)->getVelocitiesRadS_board_biasRemoved();
        m_currentOrientation.angleQuaternion = updateLaunchAngle(timestamp);
        m_currentOrientation.tiltMagnitudeDeg = computeTilt(m_currentOrientation.angleQuaternion);
    } else {
        m_currentOrientation.angularVelocity = m_hardware->getGyroscope(0)->getVelocitiesRadS_board_biasRemoved();
        m_currentOrientation.angleQuaternion = integrateGyroscope(timestamp, m_currentOrientation.angularVelocity);

        m_currentOrientation.tiltMagnitudeDeg = computeTilt(m_currentOrientation.angleQuaternion);
        // m_debug->message("%.2f\t%.2f", m_currentOrientation.tiltMagnitudeDeg, computeTilt(updateLaunchAngle(timestamp)));
    }

    Vector3D_s accel = m_hardware->getAccelerometer(0)->getAccelerationsMSS_board();
    Quaternion worldAccel = m_currentOrientation.angleQuaternion.rotate(Quaternion(accel.x, accel.y, accel.z));
    m_debug->data("%.2f\t%.2f\t%.2f", worldAccel.b, worldAccel.c, worldAccel.d);

    // quaternionToEuler(m_currentOrientation.angleQuaternion);
    // m_debug->data("%.2f\t%.2f\t%.2f", m_currentOrientation.roll, m_currentOrientation.pitch, m_currentOrientation.yaw);

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

Quaternion OrientationEstimator::updateLaunchAngle(const Timestamp_s& timestamp) {
    // Get accelerations and filter
    Vector3D_s accelerationMSS_board = m_hardware->getAccelerometer(0)->getAccelerationsMSS_board();
    if (fabs(vector3DMagnitude(accelerationMSS_board) - Constants::G_EARTH_MSS) < 0.5) {
        m_launchAngleLowPassX.update(accelerationMSS_board.x);
        m_launchAngleLowPassY.update(accelerationMSS_board.y);
        m_launchAngleLowPassZ.update(accelerationMSS_board.z);

        // Calculate rotation
        const Quaternion worldPositiveZUnit(0.0f, 0.0f, 0.0f, 1.0f);
        const Quaternion measuredGravityUnit = Quaternion(0.0f, m_launchAngleLowPassX.value(), m_launchAngleLowPassY.value(), m_launchAngleLowPassZ.value()).normalize();
        Quaternion orientation = measuredGravityUnit.rotation_between_vectors(worldPositiveZUnit); // This calls normalize

        // If for some reason result is NaN or degenerate, fall back
        if (std::isfinite(orientation.a) && std::isfinite(orientation.b) && std::isfinite(orientation.c) && std::isfinite(orientation.d)) {
            if (m_launchAngleAlarm.isAlarmFinished(timestamp.runtime_ms)) {
                m_launchAngleAlarm.startAlarm(timestamp.runtime_ms, GROUND_REFERENCE_UPDATE_DELAY);
                constexpr float changeThreshold = 0.01;
                bool aChanged = std::fabs(orientation.a - m_launchAngle.get().a) > changeThreshold;
                bool bChanged = std::fabs(orientation.b - m_launchAngle.get().b) > changeThreshold;
                bool cChanged = std::fabs(orientation.c - m_launchAngle.get().c) > changeThreshold;
                bool dChanged = std::fabs(orientation.d - m_launchAngle.get().d) > changeThreshold;

                if (aChanged || bChanged || cChanged || dChanged) {
                    m_launchAngle.set(orientation);
                    m_debug->message("Launch orientation set to %.2f", computeTilt(orientation));
                }
            }
        }
    }
    return m_launchAngle.get();
}


float OrientationEstimator::computeTilt(const Quaternion& q) {
    // Normalize quaternion to prevent drift errors
    Quaternion norm = q;
    norm.normalize();

    // Rotate body Z-axis (0, 0, 1) by quaternion
    Quaternion bodyZ(0.0f, 0.0f, 0.0f, 1.0f);
    Quaternion rotatedZ = norm.rotate(bodyZ);

    // Clamp z to valid range for acos
    float z = fmaxf(-1.0f, fminf(1.0f, rotatedZ.d));

    // Total tilt magnitude (degrees)
    return acosf(z) * 180.0f / M_PI;
}


void OrientationEstimator::updateGyroscopeBias(const Timestamp_s& timestamp) {
    GyroscopeBias_s currentGyroBias = m_gyroscopeBias.get();
    // Only allow updates to the configuration at max 1Hz
    const bool validUpdateTick = m_gyroscopeBiasAlarm.isAlarmFinished(timestamp.runtime_ms);
    if (validUpdateTick) m_gyroscopeBiasAlarm.startAlarm(timestamp.runtime_ms, GROUND_REFERENCE_UPDATE_DELAY);
    // Loop through all gyros and update their biases
    bool isUpdated = false;
    for (uint32_t i = 0; i < m_hardware->getNumGyroscopes(); i++) {
        Gyroscope* gyroscope = m_hardware->getGyroscope(i);
        const Vector3D_s angularVelocitiesRadSRaw_sensor = gyroscope->getVelocitiesRadS_raw();

        // Determine if we are moving by looking at angular acceleration
        // It's impossible to differentiate between bias and a constat velocity, so we can only look at acceleration
        Vector3D_s angularAcceleration = {
                (angularVelocitiesRadSRaw_sensor.x - m_lowPass[i].lastVelocity.x) / (float(timestamp.dt_ms) / 1000.0f),
                (angularVelocitiesRadSRaw_sensor.y - m_lowPass[i].lastVelocity.y) / (float(timestamp.dt_ms) / 1000.0f),
                (angularVelocitiesRadSRaw_sensor.z - m_lowPass[i].lastVelocity.z) / (float(timestamp.dt_ms) / 1000.0f)
            };
        m_lowPass->lastVelocity = angularVelocitiesRadSRaw_sensor;
        constexpr float motionThreshold = 2;
        bool moving = angularAcceleration.x > motionThreshold || angularAcceleration.y > motionThreshold || angularAcceleration.z > motionThreshold;

        // Low pass filter our biases if we are not moving
        if (m_motionDetector.check(!moving, timestamp.runtime_ms)) {
            // Low pass the biases to remove high frequency noise
            m_lowPass[i].x.update(angularVelocitiesRadSRaw_sensor.x);
            m_lowPass[i].y.update(angularVelocitiesRadSRaw_sensor.y);
            m_lowPass[i].z.update(angularVelocitiesRadSRaw_sensor.z);

            // Save the biases at most at 1Hz
            if (validUpdateTick) {
                const Vector3D_s gyroscopeBiasFiltered = {m_lowPass[i].x.value(), m_lowPass[i].y.value(), m_lowPass[i].z.value()};
                // Check if the value have changed
                constexpr float velocityChangeThresh = 0.0005;
                bool xChanged = std::fabs(gyroscopeBiasFiltered.x - currentGyroBias.bias[i].x) > velocityChangeThresh;
                bool yChanged = std::fabs(gyroscopeBiasFiltered.y - currentGyroBias.bias[i].y) > velocityChangeThresh;
                bool zChanged = std::fabs(gyroscopeBiasFiltered.z - currentGyroBias.bias[i].z) > velocityChangeThresh;

                // Save the new bias if they have changed
                if (xChanged || yChanged || zChanged) {
                    m_debug->message("Gyro %d bias set to:\t%.8f\t%.8f\t%.8f", int(i), gyroscopeBiasFiltered.x, gyroscopeBiasFiltered.y, gyroscopeBiasFiltered.z);

                    currentGyroBias.bias[i] = gyroscopeBiasFiltered;
                    gyroscope->setBiasOffset(gyroscopeBiasFiltered);
                    isUpdated = true;
                }
            }
        }
    }
    // Save the new bias to FRAM if they have changed
    if (isUpdated) {
        m_gyroscopeBias.set(currentGyroBias);
    }
}

void OrientationEstimator::quaternionToEuler(const Quaternion& q) {
    // Normalize the quaternion to avoid drift issues
    Quaternion norm = q;
    norm.normalize();

    // Roll (X-axis rotation)
    float sinr_cosp = 2.0f * (norm.a * norm.b + norm.c * norm.d);
    float cosr_cosp = 1.0f - 2.0f * (norm.b * norm.b + norm.c * norm.c);
    m_currentOrientation.roll = atan2f(sinr_cosp, cosr_cosp) * 180.0f / M_PI;

    // Pitch (Y-axis rotation)
    float sinp = 2.0f * (norm.a * norm.c - norm.d * norm.b);
    if (fabsf(sinp) >= 1.0f)
        m_currentOrientation.pitch = copysignf(90.0f, sinp); // use 90 degrees if out of range
    else
        m_currentOrientation.pitch = asinf(sinp) * 180.0f / M_PI;

    // Yaw (Z-axis rotation)
    float siny_cosp = 2.0f * (norm.a * norm.d + norm.b * norm.c);
    float cosy_cosp = 1.0f - 2.0f * (norm.c * norm.c + norm.d * norm.d);
    m_currentOrientation.yaw = atan2f(siny_cosp, cosy_cosp) * 180.0f / M_PI;
}
