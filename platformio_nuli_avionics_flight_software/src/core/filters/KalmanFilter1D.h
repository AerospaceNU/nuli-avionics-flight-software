#ifndef ALTITUDE_KF_H
#define ALTITUDE_KF_H

#ifdef PLATFORMIO
#include "ArduinoEigenDense.h"
#else
#include "Eigen/Dense"
#endif

class KalmanFilter1D {
public:
    KalmanFilter1D();

    void setDeltaTime(float delta_t);

    void setBarometerCovariance(float covariance);

    void setPitoCovariance(float covariance);

    void setAccelerometerCovariance(float covariance);

    void predict(double current_time);

    void predict();

    void genericUpdate(const Eigen::Matrix<float, 3, 1> &measurement, const Eigen::Matrix<float, 3, 1> &mask);

    /** @brief Call predict() first each time step. */
    void positionDataUpdate(float position);

    void accelerationDataUpdate(float acceleration);

    void positionAndAccelerationDataUpdate(float position, float acceleration);

    void velocityAndAccelerationDataUpdate(float velocity, float acceleration);

    void allDataUpdate(float position, float velocity, float acceleration);

    Eigen::Matrix<float, 3, 1> getStateVector();

    float getPosition();

    float getVelocity();

    float getAcceleration();

    void restState(float position, float velocity, float acceleration);

protected:
    // Wikipedia Kalman filter naming (F=A, H=C in state-space terms)
    Eigen::Matrix<float, 3, 1> x;               // [position, velocity, acceleration]
    Eigen::Matrix<float, 3, 3> F;               // continuous-time state transition
    Eigen::Matrix<float, 3, 3> H;               // if ever made settable, also update m_isHIdentity

    Eigen::Matrix<float, 1, 3> R_baro;          // per-sensor rows of R
    Eigen::Matrix<float, 1, 3> R_pito;
    Eigen::Matrix<float, 1, 3> R_accel;

    Eigen::Matrix<float, 3, 3> F_d; // discrete-time F

    Eigen::Matrix<float, 3, 3> R;           // measurement covariance
    Eigen::Matrix<float, 3, 3> P;           // state covariance
    Eigen::Matrix<float, 3, 3> Q;           // process covariance

    double lastTime = 0;

private:
    float processJerkStd = 1.0f;

    // Lets genericUpdate() skip the H multiplies when H is identity
    bool m_isHIdentity = false;
};



#endif //ALTITUDE_KF_H
