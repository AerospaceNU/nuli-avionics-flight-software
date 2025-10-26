#include "core/state_estimation//StateEstimatorBasic6D.h"
#include "util/Timer.h"
#include "core/transform/Vector3DTransform.h"
#include "core/transform/DiscreteRotation.h"
#include "core/transform/Quaternion.h"


#define RAD_TO_DEG_M(x) ((x) * (180.0f / M_PI))

void StateEstimatorBasic6D::setup(HardwareAbstraction* hardware, Configuration* configuration) {
    m_hardware = hardware;
    m_configuration = configuration;
    m_debug = hardware->getDebugStream();

    m_groundElevation = m_configuration->getConfigurable<GROUND_ELEVATION_c>();

    m_kalmanFilterZ.setDeltaTime(float(m_hardware->getTargetLoopTimeMs()) / 1000.0f);
    m_kalmanFilterZ.setAccelerometerCovariance(1);

    m_kalmanFilterX.setDeltaTime(float(m_hardware->getTargetLoopTimeMs()) / 1000.0f);
    m_kalmanFilterX.setAccelerometerCovariance(1);
    m_kalmanFilterX.setPitoCovariance(2);

    m_kalmanFilterY.setDeltaTime(float(m_hardware->getTargetLoopTimeMs()) / 1000.0f);
    m_kalmanFilterY.setAccelerometerCovariance(1);
    m_kalmanFilterY.setPitoCovariance(2);
}

State6D_s StateEstimatorBasic6D::update(const Timestamp_s& timestamp, const State1D_s& state1D, const Orientation_s& orientation) {
    // const Vector3D_s accelerationsMSS_board = m_hardware->getAccelerometer(0)->getAccelerationsMSS_board();
    // m_currentState6D.acceleration = QuaternionTransform(orientation.angleQuaternion).transform(accelerationsMSS_board);
    //
    // // Determine Velocity's
    // m_kalmanFilterZ.predict();
    // m_kalmanFilterZ.altitudeAndAccelerationDataUpdate(state1D.unfilteredNoOffsetAltitudeM - m_groundElevation.get(), m_currentState6D.acceleration.z - Constants::G_EARTH_MSS);
    //
    // const float vz = m_kalmanFilterZ.getVelocity();
    // const Vector3D_s worldDir = QuaternionHelper::rotateVector(orientation.angleQuaternion, {0, 0, 1});
    // const float scale = vz / worldDir.z;
    // const Vector3D_s projectedVelocity = {worldDir.x * scale, worldDir.y * scale, vz};
    //
    // m_kalmanFilterX.predict();
    // m_kalmanFilterX.velocityAndAccelerationDataUpdate(projectedVelocity.x, m_currentState6D.acceleration.x);
    // m_kalmanFilterY.predict();
    // m_kalmanFilterY.velocityAndAccelerationDataUpdate(projectedVelocity.y, m_currentState6D.acceleration.y);
    //
    // // Integrate position
    // m_currentState6D.position.x = m_kalmanFilterX.getAltitude();
    // m_currentState6D.position.y = m_kalmanFilterY.getAltitude();
    // m_currentState6D.position.z = m_kalmanFilterZ.getAltitude();

    return m_currentState6D;
}

State6D_s StateEstimatorBasic6D::getState6D() const {
    return m_currentState6D;
}
