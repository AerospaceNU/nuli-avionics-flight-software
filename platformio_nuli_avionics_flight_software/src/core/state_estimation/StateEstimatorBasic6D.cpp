#include "core/state_estimation//StateEstimatorBasic6D.h"

#define RAD_TO_DEG_M(x) ((x) * (180.0f / M_PI))

StateEstimatorBasic6D::StateEstimatorBasic6D(bool useKalman) {
    m_useKalman = useKalman;
}

void StateEstimatorBasic6D::setup(HardwareAbstraction* hardware, Configuration* configuration) {
    m_hardware = hardware;
    m_configuration = configuration;
    m_debug = hardware->getDebugStream();

    m_groundElevation = m_configuration->getConfigurable<GROUND_ELEVATION_c>();
    m_boardOrientation = m_configuration->getConfigurable<BOARD_ORIENTATION_c>();

    m_currentState6D.position.x = 0;
    m_currentState6D.position.y = 0;
    m_currentState6D.position.z = 0;

    m_kalmanFilter.setDeltaTime(float(m_hardware->getTargetLoopTimeMs()) / 1000.0f);
    m_kalmanFilter.setAccelerometerCovariance(0.0046437);
    m_kalmanFilter.setBarometerCovariance(4.7828);
}

State6D_s StateEstimatorBasic6D::update(const Timestamp_s& timestamp, const State1D_s& state1D, const Orientation_s& orientation, FlightState_e flightState) {
    const float dtSeconds = float(timestamp.dt_ms) / 1000.0f;
    const float altitudeM = state1D.unfilteredNoOffsetAltitudeM - m_groundElevation.get();
    const Vector3D_s accelerationMSS_worldFrame = getAccelerationMSS(orientation);

    if (m_useKalman) {
        m_kalmanFilter.predict();
        m_kalmanFilter.positionAndAccelerationDataUpdate(altitudeM, accelerationMSS_worldFrame.z);
        m_currentState6D.position.z = m_kalmanFilter.getPosition();
        m_currentState6D.velocity.z = m_kalmanFilter.getVelocity();
        m_currentState6D.acceleration.z = m_kalmanFilter.getAcceleration();
    } else {
        m_currentState6D.position.z = 0;
        m_currentState6D.velocity.z = 0;
        m_currentState6D.acceleration.z = 0;
    }

    if (flightState == ASCENT) {
        // ASCENT only: the projection assumes velocity along the rocket's axis, which breaks under chute
        // Low-frequency velocity from Z-axis projection
        Vector3D_s projectedVelocityMS = projectVelocities(orientation, m_currentState6D.velocity.z);

        // High-frequency velocity from integrated acceleration
        // m_integratedVelocityXY.x += accelerationMSS_worldFrame.x * dtSeconds;
        // m_integratedVelocityXY.y += accelerationMSS_worldFrame.y * dtSeconds;
        m_integratedVelocityXY.x = m_currentState6D.velocity.x + accelerationMSS_worldFrame.x * dtSeconds;
        m_integratedVelocityXY.y = m_currentState6D.velocity.y + accelerationMSS_worldFrame.y * dtSeconds;

        // Complementary filter
        constexpr float alpha = 0.1f;
        m_currentState6D.velocity.x = alpha * m_integratedVelocityXY.x + (1 - alpha) * projectedVelocityMS.x;
        m_currentState6D.velocity.y = alpha * m_integratedVelocityXY.y + (1 - alpha) * projectedVelocityMS.y;

        m_currentState6D.position.x += m_currentState6D.velocity.x * dtSeconds;
        m_currentState6D.position.y += m_currentState6D.velocity.y * dtSeconds;
        m_currentState6D.acceleration.x = accelerationMSS_worldFrame.x;
        m_currentState6D.acceleration.y = accelerationMSS_worldFrame.y;
    }

    return m_currentState6D;
}

State6D_s StateEstimatorBasic6D::getState6D() const {
    return m_currentState6D;
}

Vector3D_s StateEstimatorBasic6D::getAccelerationMSS(const Orientation_s& orientation) const {
    const Vector3D_s accelerationsMSS_board = m_hardware->getAccelerometer(0)->getAccelerationsMSS_board();
    const Quaternion accelerationsMSS_worldQ = orientation.angleQuaternion.rotate(Quaternion(accelerationsMSS_board.x, accelerationsMSS_board.y, accelerationsMSS_board.z));
    // World frame is gravity-aligned (+Z up)
    return {accelerationsMSS_worldQ.b, accelerationsMSS_worldQ.c, accelerationsMSS_worldQ.d - float(Constants::G_EARTH_MSS)};
}

Vector3D_s StateEstimatorBasic6D::projectVelocities(const Orientation_s& orientation, float velocityZ) const {
    Quaternion forwardBody(0.0f, 0.0f, 0.0f, 1.0f);

    // Use whichever board axis lies along the rocket
    uint32_t direction = m_boardOrientation.get();
    if (direction == POS_X) forwardBody = Quaternion(1, 0, 0);
    else if (direction == NEG_X) forwardBody = Quaternion(-1, 0, 0);
    else if (direction == POS_Y) forwardBody = Quaternion(0, 1, 0);
    else if (direction == NEG_Y) forwardBody = Quaternion(0, -1, 0);
    else if (direction == POS_Z) forwardBody = Quaternion(0, 0, 1);
    else if (direction == NEG_Z) forwardBody = Quaternion(0, 0, -1);

    Quaternion forwardWorld = orientation.angleQuaternion.rotate(forwardBody);

    // Scale the forward axis so its world Z matches velocityZ
    float scale = velocityZ / forwardWorld.d;

    float vx = forwardWorld.b * scale;
    float vy = forwardWorld.c * scale;
    float vz = velocityZ;

    return {vx, vy, vz};
}
