#ifndef ALTITUDE_KF_H
#define ALTITUDE_KF_H

//#include "Eigen/Eigen"
#include "ArduinoEigenDense.h"

float altitudeFromPressure(float pressure_pa);

class AltitudeKf {
public:
    AltitudeKf();

    void calculateDiscreteTimeA(float delta_t);

    void predict(double current_time);

    void genericUpdate(const Eigen::Matrix<float, 3, 1>& measurement, const Eigen::Matrix<float, 3, 3>& C);

    // This won't work if you haven't called update this time step
    void altitudeDataUpdate(float altitude);

    void accelerationDataUpdate(float acceleration);

    void dataUpdate(float altitude, float acceleration);

    Eigen::Matrix<float, 3, 1> getStateVector();

    float getAltitude() ;

protected:
    Eigen::Matrix<float, 3, 1> x;
    Eigen::Matrix<float, 3, 3> A;
    Eigen::Matrix<float, 1, 3> C_baro;
    Eigen::Matrix<float, 1, 3> C_accel;

    Eigen::Matrix<float, 3, 3> A_d; // The discrete time version

    double lastTime = 0;
};


#endif //ALTITUDE_KF_H
