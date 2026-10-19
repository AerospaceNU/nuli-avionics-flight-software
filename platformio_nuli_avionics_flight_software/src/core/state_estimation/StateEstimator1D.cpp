#include "core/state_estimation/StateEstimator1D.h"
#include "ConstantsUnits.h"
#include "core/generic_hardware/GenericHardware.h"
#include <algorithm>

void StateEstimator1D::setup(HardwareAbstraction* hardware, Configuration* configuration) {
    m_hardware = hardware;
    m_configuration = configuration;
    m_groundElevation = m_configuration->getConfigurable<GROUND_ELEVATION_c>();
    m_groundTemperature = m_configuration->getConfigurable<GROUND_TEMPERATURE_c>();
    m_boardOrientation = m_configuration->getConfigurable<BOARD_ORIENTATION_c>();

    m_kalmanFilter.setDeltaTime(m_hardware->getTargetLoopTimeS());
    m_kalmanFilter.setAccelerometerCovariance(1);

    m_boardOrientationReferenceTimer.triggerImmediately();
    m_groundElevationReferenceTimer.triggerImmediately();
}

State1D_s StateEstimator1D::update(const Timestamp_s& timestamp, const FlightState_e& flightState) {
    float pressurePa = getPressurePa();
    if (!m_barometerEverValid) return m_currentState1D;
    float accelerationMSS = getAccelerationMSS(flightState);

    if (flightState == PRE_FLIGHT) {
        updateBoardOrientationReference(timestamp);
        updateGroundElevationReference(pressurePa, timestamp);
    }
    float altitudeRawM = Barometer::calculateAltitudeM(pressurePa, m_groundTemperature.get());

    if (m_reInitializeKalman) {
        m_kalmanFilter.restState(altitudeRawM - m_groundElevation.get(), m_kalmanFilter.getVelocity(), m_kalmanFilter.getAcceleration());
        m_reInitializeKalman = false;
    }

    m_kalmanFilter.setBarometerCovariance(machBarometerCovarianceScale());
    m_kalmanFilter.predict();
    if (m_barometerValid && m_accelerationValid) {
        m_kalmanFilter.positionAndAccelerationDataUpdate(altitudeRawM - m_groundElevation.get(), accelerationMSS);
    } else if (m_barometerValid) {
        m_kalmanFilter.positionDataUpdate(altitudeRawM - m_groundElevation.get());
    } else if (m_accelerationValid) {
        m_kalmanFilter.accelerationDataUpdate(accelerationMSS);
    }

    m_currentState1D.altitudeM = m_kalmanFilter.getPosition();
    m_currentState1D.velocityMS = m_kalmanFilter.getVelocity();
    m_currentState1D.accelerationMSS = m_kalmanFilter.getAcceleration();
    m_currentState1D.unfilteredNoOffsetAltitudeM = altitudeRawM;
    m_currentState1D.barometerValid = m_barometerValid;
    return m_currentState1D;
}

State1D_s StateEstimator1D::getState1D() const {
    return m_currentState1D;
}

float StateEstimator1D::getPressurePa() {
    float pressurePa = 0;
    int32_t count = 0;
    for (int32_t i = 0; i < m_hardware->getNumBarometers(); i++) {
        Barometer* barometer = m_hardware->getBarometer(i);
        if (barometer->validReading()) {
            pressurePa += barometer->getPressurePa();
            count++;
        }
    }

    m_barometerValid = count > 0;
    if (m_barometerValid && m_barometerInvalidTicks * m_hardware->getTargetLoopTimeMs() >= BAROMETER_OUTAGE_MS) m_reInitializeKalman = true;
    m_barometerInvalidTicks = m_barometerValid ? 0 : m_barometerInvalidTicks + 1;
    if (m_barometerValid) m_barometerEverValid = true;

    if (count != 0) m_lastPressure = pressurePa / float(count);
    return m_lastPressure;
}

float StateEstimator1D::getAccelerationMSS(const FlightState_e& flightState) {
    // @todo, full scale switching, bad reading detection
    // @todo Handle multiple accelerometers: only accelerometer 0 is read
    m_currentState1D.accelerometerMagnitudeMSS = -1.0f;
    if (m_hardware->getNumAccelerometers() > 0 && m_hardware->getAccelerometer(0)->validReading()) {
        const Vector3D_s a = m_hardware->getAccelerometer(0)->getAccelerationsMSS_board();
        m_currentState1D.accelerometerMagnitudeMSS = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    }
    m_accelerationValid = true;
    if ((flightState == PRE_FLIGHT || flightState == ASCENT) && m_hardware->getNumAccelerometers() > 0) {
        m_accelerationValid = m_hardware->getAccelerometer(0)->validReading();
        switch (m_boardOrientation.get()) {
        case POS_X:
            return m_hardware->getAccelerometer(0)->getAccelerationsMSS_board().x - float(Constants::G_EARTH_MSS);
        case NEG_X:
            return -m_hardware->getAccelerometer(0)->getAccelerationsMSS_board().x - float(Constants::G_EARTH_MSS);
        case POS_Y:
            return m_hardware->getAccelerometer(0)->getAccelerationsMSS_board().y - float(Constants::G_EARTH_MSS);
        case NEG_Y:
            return -m_hardware->getAccelerometer(0)->getAccelerationsMSS_board().y - float(Constants::G_EARTH_MSS);
        case POS_Z:
            return m_hardware->getAccelerometer(0)->getAccelerationsMSS_board().z - float(Constants::G_EARTH_MSS);
        case NEG_Z:
            return -m_hardware->getAccelerometer(0)->getAccelerationsMSS_board().z - float(Constants::G_EARTH_MSS);
        case ERROR_AXIS_DIRECTION:
        default:
            return 0;
        }
    }
    return 0;
}

void StateEstimator1D::reset() {
    m_reInitializeKalman = true;
    m_needNewGroundReference = true;
    m_boardOrientationReferenceTimer.triggerImmediately();
    m_groundPressureReferenceLowPass.reset();
}

void StateEstimator1D::updateGroundElevationReference(const float pressurePa, const Timestamp_s& timestamp) {
    // Ground elevation
    if (!m_barometerValid) return;
    m_groundTemperatureReferenceLowPass.update(m_hardware->getBarometer(0)->getTemperatureK());
    m_groundPressureReferenceLowPass.update(pressurePa);
    if (!m_needNewGroundReference && std::fabs(m_kalmanFilter.getVelocity()) > GROUND_REFERENCE_FREEZE_SPEED_MS) return;
    if (m_needNewGroundReference || m_groundElevationReferenceTimer.isAlarmFinished(timestamp.runtime_ms)) {
        m_groundElevationReferenceTimer.startAlarm(timestamp.runtime_ms, GROUND_REFERENCE_UPDATE_DELAY);
        const float groundTemperature = m_groundTemperatureReferenceLowPass.value();
        const float groundElevation = Barometer::calculateAltitudeM(m_groundPressureReferenceLowPass.value(), groundTemperature);
        bool groundElevationChanged = std::fabs(m_groundElevation.get() - groundElevation) > 2.0f;
        bool groundTemperatureChanged = std::fabs(m_groundTemperature.get() - groundTemperature) > 2.0f;
        if (m_needNewGroundReference || groundElevationChanged || groundTemperatureChanged) {
            m_needNewGroundReference = false;
            m_reInitializeKalman = true;
            m_groundElevation.set(groundElevation);
            m_groundTemperature.set(groundTemperature);
            m_hardware->getDebugStream()->message("Ground elevation set to %f, %f", m_groundElevation.get(), m_groundTemperature.get());
        }
    }
}

void StateEstimator1D::updateBoardOrientationReference(const Timestamp_s& timestamp) {
    if (m_hardware->getNumAccelerometers() < 1) return;
    Accelerometer* accelerometer = m_hardware->getAccelerometer(0);
    const Vector3D_s a = accelerometer->getAccelerationsMSS_board();
    if (accelerometer->validReading() && std::fabs(std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z) - Constants::G_EARTH_MSS) < 1.0) {
        m_lowPassAX.update(a.x);
        m_lowPassAY.update(a.y);
        m_lowPassAZ.update(a.z);
    }
    if (!m_lowPassAX.isInitialized() || !m_boardOrientationReferenceTimer.isAlarmFinished(timestamp.runtime_ms)) return;
    m_boardOrientationReferenceTimer.startAlarm(timestamp.runtime_ms, GROUND_REFERENCE_UPDATE_DELAY);

    const AxisDirection direction = dominantAxis({m_lowPassAX.value(), m_lowPassAY.value(), m_lowPassAZ.value()});
    if (m_boardOrientation.get() != direction) {
        m_boardOrientation.set(direction);
        m_hardware->getDebugStream()->message("Board orientation set to %s", axisDirectionName(direction));
    }
}

AxisDirection StateEstimator1D::dominantAxis(const Vector3D_s& vector) {
    const float absX = std::fabs(vector.x);
    const float absY = std::fabs(vector.y);
    const float absZ = std::fabs(vector.z);
    if (absX > absY && absX > absZ) return vector.x > 0 ? POS_X : NEG_X;
    if (absY > absZ) return vector.y > 0 ? POS_Y : NEG_Y;
    return vector.z > 0 ? POS_Z : NEG_Z;
}

float StateEstimator1D::machBarometerCovarianceScale() {
    if (!m_accelerationValid) return 1.0f;
    const float machRamp = std::max(0.0f, std::min(1.0f, (m_kalmanFilter.getVelocity() - MACH_RAMP_START_MS) / (MACH_RAMP_END_MS - MACH_RAMP_START_MS)));
    return 1.0f + machRamp * (MACH_BAROMETER_COVARIANCE_SCALE - 1.0f);
}
