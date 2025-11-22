#include "core/state_estimation//StateEstimatorBasic6D.h"
#include "util/Timer.h"
#include "core/transform/Vector3DTransform.h"
#include "core/transform/DiscreteRotation.h"

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
    m_kalmanFilter.setAccelerometerCovariance(1);
}

State6D_s StateEstimatorBasic6D::update(const Timestamp_s& timestamp, const State1D_s& state1D, const Orientation_s& orientation, FlightState_e flightState) {
    // This is the data we have available
    const float dtSeconds = timestamp.dt_ms / 1000.0f;
    const float altitudeM = state1D.unfilteredNoOffsetAltitudeM - m_groundElevation.get();
    const Vector3D_s accelerationMSS_worldFrame = getAccelerationMSS(orientation);

    // Determine Z axis state. This is a function of altitudeM and accelerationMSS_worldFrame
    if (m_useKalman) {
        // Implement kalman filter here, Matthew
        m_currentState6D.position.z = 0;
        m_currentState6D.velocity.z = 0;
        m_currentState6D.acceleration.z = 0;
    } else {
        // Implement fixed gain observer here, Julia
        m_currentState6D.position.z = 0;
        m_currentState6D.velocity.z = 0;
        m_currentState6D.acceleration.z = 0;
    }
    m_kalmanFilter.predict();
    m_kalmanFilter.positionAndAccelerationDataUpdate(altitudeM, accelerationMSS_worldFrame.z);
    m_currentState6D.position.z = m_kalmanFilter.getPosition();
    m_currentState6D.velocity.z = m_kalmanFilter.getVelocity();
    m_currentState6D.acceleration.z = m_kalmanFilter.getAcceleration();


    // Determine X/Y axis state
    // Project Z velocity determined by the kalman filter or fixed gain observer, Xiaofu
    Vector3D_s projectedVelocityMS = projectVelocities(orientation, m_currentState6D.velocity.z);
    // Implement complementary filter here. This is a function of projectedVelocityMS and accelerationMSS_worldFrame
    if (flightState != PRE_FLIGHT) {
        m_currentState6D.position.x += projectedVelocityMS.x * dtSeconds;
        m_currentState6D.position.y += projectedVelocityMS.y * dtSeconds;
    }
    m_currentState6D.velocity.x = projectedVelocityMS.x;
    m_currentState6D.velocity.y = projectedVelocityMS.y;
    m_currentState6D.acceleration.x = accelerationMSS_worldFrame.x;
    m_currentState6D.acceleration.y = accelerationMSS_worldFrame.y;

    return m_currentState6D;
}

State6D_s StateEstimatorBasic6D::getState6D() const {
    return m_currentState6D;
}

Vector3D_s StateEstimatorBasic6D::getAccelerationMSS(const Orientation_s& orientation) const {
    const Vector3D_s accelerationsMSS_board = m_hardware->getAccelerometer(0)->getAccelerationsMSS_board();
    const Quaternion accelerationsMSS_worldQ = orientation.angleQuaternion.rotate(Quaternion(accelerationsMSS_board.x, accelerationsMSS_board.y, accelerationsMSS_board.z));
    return {accelerationsMSS_worldQ.b, accelerationsMSS_worldQ.c, accelerationsMSS_worldQ.d - float(Constants::G_EARTH_MSS)};
}

Vector3D_s StateEstimatorBasic6D::projectVelocities(const Orientation_s& orientation, float velocityZ) const {
    Quaternion forwardBody(0.0f, 0.0f, 0.0f, 1.0f);

    // We actually want to compare against whatever axis we think is along the length of the rocket
    uint32_t direction = m_boardOrientation.get();
    if (direction == POS_X) forwardBody = Quaternion(-1, 0, 0);
    else if (direction == NEG_X) forwardBody = Quaternion(1, 0, 0);
    else if (direction == POS_Y) forwardBody = Quaternion(0, -1, 0);
    else if (direction == NEG_Y) forwardBody = Quaternion(0, 1, 0);
    else if (direction == POS_Z) forwardBody = Quaternion(0, 0, -1);
    else if (direction == NEG_Z) forwardBody = Quaternion(0, 0, 1);
    // Quaternion forwardBody(1, 0, 0); // forward along body Z

    // Rotate forward vector to world frame
    Quaternion forwardWorld = orientation.angleQuaternion.rotate(forwardBody);

    // Compute scale factor to match vertical component
    // forwardWorld.d is Z in world frame
    float scale = velocityZ / forwardWorld.d;

    // Compute full 3D velocity in world frame
    float vx = forwardWorld.b * scale; // world X
    float vy = forwardWorld.c * scale; // world Y
    float vz = velocityZ; // world Z (already known)

    return {vx, vy, vz};
}
