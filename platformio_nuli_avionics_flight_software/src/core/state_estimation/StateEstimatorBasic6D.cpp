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
}

State6D_s StateEstimatorBasic6D::update(const Timestamp_s& timestamp, const State1D_s& state1D, const Orientation_s& orientation) {
    // This is the data we have available
    const float altitudeM = state1D.unfilteredNoOffsetAltitudeM - m_groundElevation.get();
    const Vector3D_s accelerationMSS_worldFrame = getAccelerationMSS(orientation);

    // Determine Z axis state. This is a function of altitudeM and accelerationMSS_worldFrame
    if (m_useKalman) {
        // Implement kalman filter here
        m_currentState6D.position.z = 0;
        m_currentState6D.velocity.z = 0;
        m_currentState6D.acceleration.z = 0;
    } else {
        // Implement fixed gain observer here
        m_currentState6D.position.z = 0;
        m_currentState6D.velocity.z = 0;
        m_currentState6D.acceleration.z = 0;
    }

    // Determine X/Y axis state
    // Project Z velocity determined by the kalman filter or fixed gain observer
    Vector3D_s projectedVelocityMS = projectVelocities(orientation, m_currentState6D.velocity.z);
    // Implement complementary filter here. This is a function of projectedVelocityMS and accelerationMSS_worldFrame
    m_currentState6D.position.x = 0;
    m_currentState6D.position.y = 0;
    m_currentState6D.velocity.x = 0;
    m_currentState6D.velocity.y = 0;
    m_currentState6D.acceleration.x = 0;
    m_currentState6D.acceleration.y = 0;

    return m_currentState6D;
}

State6D_s StateEstimatorBasic6D::getState6D() const {
    return m_currentState6D;
}

Vector3D_s StateEstimatorBasic6D::getAccelerationMSS(const Orientation_s& orientation) const {
    const Vector3D_s accelerationsMSS_board = m_hardware->getAccelerometer(0)->getAccelerationsMSS_board();
    const Quaternion accelerationsMSS_worldQ = orientation.angleQuaternion.rotate(Quaternion(accelerationsMSS_board.x, accelerationsMSS_board.y, accelerationsMSS_board.z));
    return  {accelerationsMSS_worldQ.b, accelerationsMSS_worldQ.c, accelerationsMSS_worldQ.d};
}

Vector3D_s StateEstimatorBasic6D::projectVelocities(const Orientation_s& orientation, float velocityZ) const {
    Quaternion forwardBody(0, 0, 0, 1); // forward along body Z

    // Rotate forward vector to world frame
    Quaternion forwardWorld = orientation.angleQuaternion.rotate(forwardBody);

    // Compute scale factor to match vertical component
    // forwardWorld.d is Z in world frame
    float scale = m_currentState6D.velocity.z / forwardWorld.d;

    // Compute full 3D velocity in world frame
    float vx = forwardWorld.b * scale; // world X
    float vy = forwardWorld.c * scale; // world Y
    float vz = m_currentState6D.velocity.z; // world Z (already known)

    return {vx, vy, vz};
}

