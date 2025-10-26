//
// Created by nathan on 5/18/25.
//

#ifndef ALTITUDE_KF_H
#define ALTITUDE_KF_H

#ifdef PLATFORMIO
#include "ArduinoEigenDense.h"
#else
#include "Eigen/Dense"
#endif

float altitudeFromPressure(float pressure_pa);

class AltitudeKf {
public:
    AltitudeKf();

    void setDeltaTime(float delta_t);

    void setBarometerCovariance(float covariance);

    void setPitoCovariance(float covariance);

    void setAccelerometerCovariance(float covariance);

    void predict(double current_time);

    void predict();

    /**
     * Runs the filter update
     * @param measurement data [p,v,a]
     * @param Observation matrix for this measurement
     * @param Measurement covariance
     */
    void genericUpdate(const Eigen::Matrix<float, 3, 1> &measurement, const Eigen::Matrix<float, 3, 1> &mask);

    // This won't work if you haven't called predict this time step
    void altitudeDataUpdate(float altitude);

    void accelerationDataUpdate(float acceleration);

    void altitudeAndAccelerationDataUpdate(float altitude, float acceleration);

    void velocityAndAccelerationDataUpdate(float velocity, float acceleration);

    void allDataUpdate(float altitude, float velocity, float acceleration);

    Eigen::Matrix<float, 3, 1> getStateVector();

    float getAltitude();

    float getVelocity();

    float getAcceleration();

    void restState(float altitude, float velocity, float acceleration);

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


#endif //ALTITUDE_KF_H
