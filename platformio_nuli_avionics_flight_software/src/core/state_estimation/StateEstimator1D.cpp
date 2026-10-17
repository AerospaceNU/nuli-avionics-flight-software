#include "core/state_estimation/StateEstimator1D.h"
#include "ConstantsUnits.h"
#include "core/generic_hardware/GenericHardware.h"
#include <algorithm>

constexpr ConfigurationID_t StateEstimator1D::REQUIRED_CONFIGS[];

void StateEstimator1D::setup(HardwareAbstraction* hardware, Configuration* configuration) {
    m_hardware = hardware;
    m_configuration = configuration;
    m_groundElevation = m_configuration->getConfigurable<GROUND_ELEVATION_c>();
    m_groundTemperature = m_configuration->getConfigurable<GROUND_TEMPERATURE_c>();
    m_boardOrientation = m_configuration->getConfigurable<BOARD_ORIENTATION_c>();

    m_kalmanFilter.setDeltaTime(float(m_hardware->getTargetLoopTimeMs()) / 1000.0f);
    m_kalmanFilter.setAccelerometerCovariance(1);

    // Fire both alarms on the first update
    m_boardOrientationReferenceTimer.startAlarm(0, 0);
    m_groundElevationReferenceTimer.startAlarm(0, 0);
}

State1D_s StateEstimator1D::update(const Timestamp_s& timestamp, const FlightState_e& flightState) {
    float pressurePa = getPressurePa();
    float accelerationMSS = getAccelerationMSS(flightState);

    if (m_barometerValid && m_barometerInvalidTicks >= BAROMETER_INVALID_REPORT_TICKS) m_hardware->getDebugStream()->message("Barometer reading valid again");
    m_barometerInvalidTicks = m_barometerValid ? 0 : m_barometerInvalidTicks + 1;
    if (m_barometerInvalidTicks == BAROMETER_INVALID_REPORT_TICKS) m_hardware->getDebugStream()->warn("No valid barometer reading");
    if (m_barometerValid) m_barometerEverValid = true;
    if (!m_barometerEverValid) {
        m_currentState1D = {};
        return m_currentState1D;
    }

    if (flightState == PRE_FLIGHT) {
        updateBoardOrientationReference(timestamp);
        if (m_barometerValid) updateGroundElevationReference(pressurePa, timestamp);
    }
    float altitudeRawM = Barometer::calculateAltitudeM(pressurePa, m_groundTemperature.get());

    if (m_reInitializeKalman) {
        m_kalmanFilter.restState(altitudeRawM - m_groundElevation.get(), m_kalmanFilter.getVelocity(), m_kalmanFilter.getAcceleration());
        m_reInitializeKalman = false;
    }

    // Trust the baro less near mach: ramp over 250-350 m/s, ~0.3x baro gain at the top (as AerospaceNU/stm32-avionics)
    const float machRamp = std::max(0.0f, std::min(1.0f, (m_kalmanFilter.getVelocity() - 250.0f) / 100.0f));
    m_kalmanFilter.setBarometerCovariance(1.0f + machRamp * (1.0f / 0.3f - 1.0f));

    m_kalmanFilter.predict();
    if (m_barometerValid) {
        m_kalmanFilter.positionAndAccelerationDataUpdate(altitudeRawM - m_groundElevation.get(), accelerationMSS);
    } else {
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
    if (count == 0) {
        return m_lastPressure;
    }

    m_lastPressure = pressurePa / float(count);
    return m_lastPressure;
}

float StateEstimator1D::getAccelerationMSS(const FlightState_e& flightState) const {
    if ((flightState == PRE_FLIGHT || flightState == ASCENT) && m_hardware->getNumAccelerometers() > 0) {
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
    m_groundPressureReferenceLowPass.reset();
}

void StateEstimator1D::updateGroundElevationReference(const float pressurePa, const Timestamp_s& timestamp) {
    m_groundTemperatureReferenceLowPass.update(m_hardware->getBarometer(0)->getTemperatureK());
    m_groundPressureReferenceLowPass.update(pressurePa);
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
    Vector3D_s accelerations = m_hardware->getAccelerometer(0)->getAccelerationsMSS_board();
    if (std::fabs(std::sqrt(accelerations.x * accelerations.x + accelerations.y * accelerations.y + accelerations.z * accelerations.z) - Constants::G_EARTH_MSS) < 1.0) {
        m_lowPassAX.update(accelerations.x);
        m_lowPassAY.update(accelerations.y);
        m_lowPassAZ.update(accelerations.z);
    }
    if (!m_lowPassAX.isInitialized()) return;
    accelerations = {m_lowPassAX.value(), m_lowPassAY.value(), m_lowPassAZ.value()};

    if (m_needNewGroundReference || m_boardOrientationReferenceTimer.isAlarmFinished(timestamp.runtime_ms)) {
        m_boardOrientationReferenceTimer.startAlarm(timestamp.runtime_ms, GROUND_REFERENCE_UPDATE_DELAY);
        const float absX = fabs(accelerations.x);
        const float absY = fabs(accelerations.y);
        const float absZ = fabs(accelerations.z);
        AxisDirection dir;
        if (absX > absY && absX > absZ) {
            dir = (accelerations.x > 0) ? POS_X : NEG_X;
        } else if (absY > absZ) {
            dir = (accelerations.y > 0) ? POS_Y : NEG_Y;
        } else {
            dir = (accelerations.z > 0) ? POS_Z : NEG_Z;
        }
        if (m_needNewGroundReference || m_boardOrientation.get() != dir) {
            m_boardOrientation.set(dir);
            const char* name = (dir == POS_X) ? "POS_X" : (dir == NEG_X) ? "NEG_X" : (dir == POS_Y) ? "POS_Y" : (dir == NEG_Y) ? "NEG_Y" : (dir == POS_Z) ? "POS_Z" : "NEG_Z";
            m_hardware->getDebugStream()->message("Board orientation set to %s", name);
        }
    }
}
