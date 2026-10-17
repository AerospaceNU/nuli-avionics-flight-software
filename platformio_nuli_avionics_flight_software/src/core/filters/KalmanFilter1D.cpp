#include "KalmanFilter1D.h"
#include <cmath>
#include <algorithm>

KalmanFilter1D::KalmanFilter1D() {
    x = {0, 0, 0};

    // F_d in setDeltaTime() is the hand-derived discretization of this F: change both together
    F = Eigen::Matrix<float, 3, 3>{{0, 1, 0},
                                   {0, 0, 1},
                                   {0, 0, 0},
    };

    H = Eigen::Matrix<float, 3, 3>{{1, 0, 0},       // Barometer
                                   {0, 1, 0},       // Pitot tube
                                   {0, 0, 1},       // Accelerometer
    };
    m_isHIdentity = H.isIdentity();

    P = Eigen::Matrix<float, 3, 3>::Identity() * 1000.0f;

    // Placeholder only: setDeltaTime() recomputes Q from processJerkStd
    Q = Eigen::Matrix<float, 3, 3>{{0.1, 0.1, 0},
                                   {0.1, 0.1, 0},
                                   {0,   0,   0.01}
    };

    setBarometerCovariance(1);          // m
    setPitoCovariance(5);               // m/s
    setAccelerometerCovariance(0.1);    // m/s^2
}

void KalmanFilter1D::setDeltaTime(float delta_t) {
    // ArduinoEigen has no matrix exponential, so the discretization is written out by hand
    F_d = Eigen::Matrix<float, 3, 3>{{1, delta_t, 0.5f * std::pow(delta_t, 2.0f)},
                                     {0, 1,       delta_t},
                                     {0, 0,       1}
    };

    // Q from jerk spectral density q = sigma^2
    const float q = processJerkStd * processJerkStd;
    const float dt = delta_t;
    const float dt2 = dt*dt;
    const float dt3 = dt2*dt;
    const float dt4 = dt3*dt;
    const float dt5 = dt4*dt;

    Q = Eigen::Matrix<float, 3, 3>{
            {q * dt5 / 20.0f, q * dt4 / 8.0f, q * dt3 / 6.0f},
            {q * dt4 / 8.0f,  q * dt3 / 3.0f, q * dt2 / 2.0f},
            {q * dt3 / 6.0f,  q * dt2 / 2.0f, q * dt}
    };
}

void KalmanFilter1D::setBarometerCovariance(float covariance) {
    R_baro = {covariance, 0, 0};
}

void KalmanFilter1D::setPitoCovariance(float covariance) {
    R_pito = {0, covariance, 0};
}

void KalmanFilter1D::setAccelerometerCovariance(float covariance) {
    R_accel = {0, 0, covariance};
}

void KalmanFilter1D::predict(double current_time) {
    auto delta_t = std::min((float) (current_time - lastTime), 1.0f); // clamp covers the first call; calling slower than 1 Hz breaks the filter
    lastTime = current_time;

    setDeltaTime(delta_t);

    predict();
}

void KalmanFilter1D::predict() {
    x = F_d * x;

    P = F_d * P * F_d.transpose() + Q;
}

void KalmanFilter1D::genericUpdate(const Eigen::Matrix<float, 3, 1> &measurement, const Eigen::Matrix<float, 3, 1> &mask) {
    R.row(0) = R_baro;
    R.row(1) = R_pito;
    R.row(2) = R_accel;

    Eigen::Matrix<float, 3, 3> I;
    I.setIdentity();

    if (m_isHIdentity) {
        Eigen::Matrix<float, 3, 1> innovation = (measurement - x).cwiseProduct(mask);
        Eigen::Matrix<float, 3, 3> S = P + R;
        Eigen::Matrix<float, 3, 3> K = P * S.inverse();
        x = x + K * innovation;
        P = (I - K) * P;
    } else {
        Eigen::Matrix<float, 3, 1> innovation = measurement - H * x;
        innovation = innovation.cwiseProduct(mask);             // mask drops states not measured this update

        Eigen::Matrix<float, 3, 3> S = H * P * H.transpose() + R;

        Eigen::Matrix<float, 3, 3> K = P * H.transpose() * S.inverse();

        x = x + K * innovation;

        P = (I - K * H) * P;
    }
}

void KalmanFilter1D::positionDataUpdate(float position) {
    genericUpdate({position, 0, 0}, {1, 0, 0});
}

void KalmanFilter1D::accelerationDataUpdate(float acceleration) {
    genericUpdate({0, 0, acceleration}, {0, 0, 1});
}

void KalmanFilter1D::positionAndAccelerationDataUpdate(float position, float acceleration) {
    genericUpdate({position, 0, acceleration}, {1, 0, 1});
}

void KalmanFilter1D::velocityAndAccelerationDataUpdate(float velocity, float acceleration) {
    genericUpdate({0, velocity, acceleration}, {0, 1, 1});
}


void KalmanFilter1D::allDataUpdate(float position, float velocity, float acceleration) {
    genericUpdate({position, velocity, acceleration}, {1, 1, 1});
}

Eigen::Matrix<float, 3, 1> KalmanFilter1D::getStateVector() {
    return x;
}

float KalmanFilter1D::getPosition() {
    return getStateVector()[0];
}

float KalmanFilter1D::getVelocity() {
    return getStateVector()[1];
}

float KalmanFilter1D::getAcceleration() {
    return getStateVector()[2];
}

void KalmanFilter1D::restState(float position, float velocity, float acceleration) {
    x = {position, velocity, acceleration};
}
