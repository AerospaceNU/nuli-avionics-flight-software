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

    // This won't work if you haven't called predict this time step
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
    // I use the naming convention from the wikipedia page on kalman filters
    // If you're familiar with state space controllers, F=A, H=C
    Eigen::Matrix<float, 3, 1> x;               // State
    Eigen::Matrix<float, 3, 3> F;               // The state transition matrix
    Eigen::Matrix<float, 3, 3> H;               // The observation matrix

    Eigen::Matrix<float, 1, 3> R_baro;          // Measurement covariance for each sensor
    Eigen::Matrix<float, 1, 3> R_pito;
    Eigen::Matrix<float, 1, 3> R_accel;

    Eigen::Matrix<float, 3, 3> F_d; // The discrete time version of the state transition matrix

    Eigen::Matrix<float, 3, 3> R;           // The sensor covariance matrix
    Eigen::Matrix<float, 3, 3> P;           // The current state covariance
    Eigen::Matrix<float, 3, 3> Q;           // The process covariance

    double lastTime = 0;
};


#endif //position_KF_H
