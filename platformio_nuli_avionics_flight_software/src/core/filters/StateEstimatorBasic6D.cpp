#include "StateEstimatorBasic6D.h"

#define RAD_TO_DEG_M(x) ((x) * (180.0f / M_PI))

void StateEstimatorBasic6D::setup(HardwareAbstraction* hardware, Configuration* configuration) {
    m_hardware = hardware;
    m_configuration = configuration;
    m_groundElevation = m_configuration->getConfigurable<GROUND_ELEVATION_c>();
    m_boardOrientation = m_configuration->getConfigurable<BOARD_ORIENTATION_c>();
    m_currentState6D.position.x = 0;
    m_currentState6D.position.y = 0;
}

State6D_s StateEstimatorBasic6D::loopOnce(const Timestamp_s& timestamp, const State1D_s& state1D, const FlightState_e& flightState) {
    m_currentState6D.acceleration.x = 0;

    const Vector3D_s orientation = integrateGyroscope(m_hardware->getGyroscope(0)->getVelocitiesRadS());
    // m_hardware->getDebugStream()->message("Roll, pitch, yaw: \t%.2f\t%.2f\t%.2f", RAD_TO_DEG_M(orientation.x), RAD_TO_DEG_M(orientation.y), RAD_TO_DEG_M(orientation.z));

    const Vector3D_s velWorld = projectVelocityToWorld(orientation, state1D.velocityMS);
    m_currentState6D.velocity.x = velWorld.x;
    m_currentState6D.velocity.y = velWorld.y;
    m_currentState6D.velocity.z = velWorld.z;

    m_currentState6D.position.x += m_currentState6D.velocity.x * float(m_hardware->getTargetLoopTimeMs()) / 1000.0f;
    m_currentState6D.position.y += m_currentState6D.velocity.y * float(m_hardware->getTargetLoopTimeMs()) / 1000.0f;
    m_currentState6D.position.z = state1D.altitudeM;

    // m_hardware->getDebugStream()->message("x,y,z: \t%.2f\t%.2f\t%.2f", m_currentState6D.velocity.x, m_currentState6D.velocity.y, m_currentState6D.velocity.z);
    // m_hardware->getDebugStream()->message("x,y,z: \t%.2f\t%.2f\t%.2f", m_currentState6D.position.x, m_currentState6D.position.y, m_currentState6D.position.z);

    return m_currentState6D;
}

State6D_s StateEstimatorBasic6D::getState6D() const {
    return m_currentState6D;
}

Vector3D_s StateEstimatorBasic6D::integrateGyroscope(const Vector3D_s& angularVelocity) const {
    const float dt_s = float(m_hardware->getTargetLoopTimeMs()) / 1000.0f;
    // Persistent orientation state
    static Quaternion q = {1.0f, 0.0f, 0.0f, 0.0f};     // @todo move to member varable

    // --- Quaternion integration ---
    const float half_dt = 0.5f * dt_s;

    Quaternion dq{};
    dq.w = -half_dt * (q.x * angularVelocity.x + q.y * angularVelocity.y + q.z * angularVelocity.z);
    dq.x = half_dt * (q.w * angularVelocity.x + q.y * angularVelocity.z - q.z * angularVelocity.y);
    dq.y = half_dt * (q.w * angularVelocity.y - q.x * angularVelocity.z + q.z * angularVelocity.x);
    dq.z = half_dt * (q.w * angularVelocity.z + q.x * angularVelocity.y - q.y * angularVelocity.x);

    q.w += dq.w;
    q.x += dq.x;
    q.y += dq.y;
    q.z += dq.z;

    // Normalize quaternion to prevent drift
    const float norm = sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    q.w /= norm;
    q.x /= norm;
    q.y /= norm;
    q.z /= norm;

    // --- Convert quaternion to Euler angles (roll/pitch/yaw) ---
    Vector3D_s e{};
    e.x = atan2f(2.0f * (q.w * q.x + q.y * q.z),
                 1.0f - 2.0f * (q.x * q.x + q.y * q.y)); // Roll

    e.y = asinf(2.0f * (q.w * q.y - q.z * q.x)); // Pitch

    e.z = atan2f(2.0f * (q.w * q.z + q.x * q.y),
                 1.0f - 2.0f * (q.y * q.y + q.z * q.z)); // Yaw

    return e;
}

Vector3D_s StateEstimatorBasic6D::projectVelocityToWorld(const Vector3D_s& euler, const float bodyVelocity) const {
    // Extract pitch and yaw (roll doesn't affect direction of travel)
    const float pitch = euler.y;
    const float yaw   = euler.z;

    // Direction vector in world frame for body +Z axis
    // Using aerospace convention: yaw around Z, pitch around Y
    Vector3D_s dir{};
    dir.x = sinf(yaw) * cosf(pitch);
    dir.y = -sinf(pitch);         // Negative because pitch up means pointing more -Y in ENU-like frame
    dir.z = cosf(yaw) * cosf(pitch);

    // Scale by body-axis velocity
    dir.x *= bodyVelocity;
    dir.y *= bodyVelocity;
    dir.z *= bodyVelocity;

    return dir;
}

Vector3D_s StateEstimatorBasic6D::projectAccelToWorld(const Vector3D_s& euler, const Vector3D_s& accelBody) const {
    const float roll  = euler.x;
    const float pitch = euler.y;
    const float yaw   = euler.z;

    const float cr = cosf(roll);
    const float sr = sinf(roll);
    const float cp = cosf(pitch);
    const float sp = sinf(pitch);
    const float cy = cosf(yaw);
    const float sy = sinf(yaw);

    // Rotation matrix: world_from_body (Z-Y-X)
    const float R[3][3] = {
        { cy*cp, cy*sp*sr - sy*cr, cy*sp*cr + sy*sr },
        { sy*cp, sy*sp*sr + cy*cr, sy*sp*cr - cy*sr },
        { -sp,   cp*sr,            cp*cr           }
    };

    Vector3D_s accelWorld{};
    accelWorld.x = R[0][0]*accelBody.x + R[0][1]*accelBody.y + R[0][2]*accelBody.z;
    accelWorld.y = R[1][0]*accelBody.x + R[1][1]*accelBody.y + R[1][2]*accelBody.z;
    accelWorld.z = R[2][0]*accelBody.x + R[2][1]*accelBody.y + R[2][2]*accelBody.z;

    return accelWorld;
}