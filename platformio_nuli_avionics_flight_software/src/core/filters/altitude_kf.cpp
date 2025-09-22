//
// Created by nathan on 5/18/25.
//

#include "altitude_kf.h"

#if __has_include("unsupported/Eigen/MatrixFunctions")

#include "unsupported/Eigen/MatrixFunctions"

#define EIGEN_UNSUPPORTED 1
#endif

#if  __has_include("iostream")

#include "iostream"

#endif

float altitudeFromPressure(float pressure_pa) {
    // https://en.wikipedia.org/wiki/Pressure_altitude, converted to use m and pa
    float pressure_mbar = pressure_pa / 100.0f;
    float altitude_ft = 145366.45f * (1 - std::pow((pressure_mbar / 1013.25f), 0.190284f));
    return altitude_ft * 0.3048f;
}

AltitudeKf::AltitudeKf() {
    // States are [position, velocity, acceleration]
    x = {0, 0, 0};

    // Continuous time state transitions are pretty straightforward
    // DON'T CHANGE THIS WITHOUT USING THE MATRIX EXPONENTIAL, OR FIXING setDeltaTime
    F = Eigen::Matrix<float, 3, 3>{{0, 1, 0},
                                   {0, 0, 1},
                                   {0, 0, 0},
    };
    // As long as we measure position, (which we do), the system is observable.  Observability is so easy

    // Where we're going, we don't need a B matrix

    // Smush all the sensor H matrices together
    H = Eigen::Matrix<float, 3, 3>{{1, 0, 0},       // Barometer
                                   {0, 1, 0},       // Pito tube
                                   {0, 0, 1},       // Accelerometer
    };

    // IDK if we actually want to start with zero covariance, but I think it should be numerically stable
    P.setZero();

    // Take a wild guess at the process covariance (I really need to figure out what these actually mean)
    // Larger means that there's more uncertainty in the state model, so the filter should trust the measurement more
    // This matrix needs to stay symmetric.  The diagonals are the variances for each individual state
    // TODO: Figure out what all these numbers really do, because I don't have an intuitive understanding yet.  Tuning this correctly matters a LOT
    Q = Eigen::Matrix<float, 3, 3>{{0.1, 0.1, 0},
                                   {0.1, 0.1, 0},
                                   {0,   0,   0.01}
    };

    // Set default measurement covariances
    setBarometerCovariance(1);          // Measurement covariance in meters
    setPitoCovariance(5);               // m/s
    setAccelerometerCovariance(0.1);    // m/s^2
}

void AltitudeKf::setDeltaTime(float delta_t) {
// #ifdef EIGEN_UNSUPPORTED
//     F_d = (F * delta_t).exp();
// #elif
    // Can't use matrix exponential on embedded systems, because that's not in ArduinoEigen
    // Instead, we calculate this ourselves
    F_d = Eigen::Matrix<float, 3, 3>{{1, delta_t, 0.5f * std::pow(delta_t, 2.0f)},
                                     {0, 1,       delta_t},
                                     {0, 0,       1}
    };
// #endif
}

void AltitudeKf::setBarometerCovariance(float covariance) {
    R_baro = {covariance, 0, 0};
}

void AltitudeKf::setPitoCovariance(float covariance) {
    R_pito = {0, covariance, 0};
}

void AltitudeKf::setAccelerometerCovariance(float covariance) {
    R_accel = {0, 0, covariance};
}

void AltitudeKf::predict(double current_time) {
    // Calculate time since last loop
    auto delta_t = std::min((float) (current_time - lastTime), 1.0f); // The clamping should only matter the first time through.  If you call this slower than once per second there will be a problem
    lastTime = current_time;

    // Discretize
    setDeltaTime(delta_t);

    // Predict
    predict();
}

void AltitudeKf::predict() {
    // Move state estimate forward in time
    x = F_d * x;

    // Move state covariance forward in time
    P = F_d * P * F_d.transpose() + Q;
}

void AltitudeKf::genericUpdate(const Eigen::Matrix<float, 3, 1> &measurement, const Eigen::Matrix<float, 3, 1> &mask) {
    // Update R matrix
    R.row(0) = R_baro;
    R.row(1) = R_pito;
    R.row(2) = R_accel;

    // Compute innovation
    Eigen::Matrix<float, 3, 1> innovation = measurement - H * x;  // Innovation is [pos; vel; accel]
    innovation = innovation.cwiseProduct(mask);             // I think this is a fine way to handle situations where we don't get all the measurements at once, but IDK really

    // Calculate innovation covariance
    Eigen::Matrix<float, 3, 3> S = H * P * H.transpose() + R;

    // Calculate gain
    Eigen::Matrix<float, 3, 3> K = P * H.transpose() * S.inverse();

    // Calculate new state estimate
    x = x + K * innovation;

    // Update state covariance
    Eigen::Matrix<float, 3, 3> I;
    I.setIdentity();
    P = (I - K * H) * P;
}

void AltitudeKf::altitudeDataUpdate(float altitude) {
    genericUpdate({altitude, 0, 0}, {1, 0, 0});
}

void AltitudeKf::accelerationDataUpdate(float acceleration) {
    genericUpdate({0, 0, acceleration}, {0, 0, 1});
}

void AltitudeKf::altitudeAndAccelerationDataUpdate(float altitude, float acceleration) {
    genericUpdate({altitude, 0, acceleration}, {1, 0, 1});
}

void AltitudeKf::allDataUpdate(float altitude, float velocity, float acceleration) {
    genericUpdate({altitude, velocity, acceleration}, {1, 1, 1});
}

Eigen::Matrix<float, 3, 1> AltitudeKf::getStateVector() {
    return x;
}

float AltitudeKf::getAltitude() {
    return getStateVector()[0];
}

float AltitudeKf::getVelocity() {
    return getStateVector()[1];
}

float AltitudeKf::getAcceleration() {
    return getStateVector()[2];
}





// #include "altitude_kf.h"
//
// //#include "unsupported/Eigen/MatrixFunctions"
// //#include ArduinoEigenDense
//
// float altitudeFromPressure(float pressure_pa) {
//     // https://en.wikipedia.org/wiki/Pressure_altitude, converted to use m and pa
//     float pressure_mbar = pressure_pa / 100.0f;
//     float altitude_ft = 145366.45f * (1 - std::pow((pressure_mbar / 1013.25f), 0.190284f));
//     return altitude_ft * 0.3048f;
// }
//
// AltitudeKf::AltitudeKf() {
//     // States are [position, velocity, acceleration]
//     x = {0, 0, 0};
//
//     // Continuous time state transitions are pretty straightforward
//     A = Eigen::Matrix<float, 3, 3>{{0, 1, 0},
//                                    {0, 0, 1},
//                                    {0, 0, 0}
//     };
//
//     // Where we're going, we don't need a B matrix
//
//     // Each sensor gets a different C matrix
//     C_baro = {1, 0, 0};
//     C_accel = {0, 0, 1};
//
//     // As long as we measure position, (which we do), the system is observable.  Observability is so easy
// }
//
// void AltitudeKf::calculateDiscreteTimeA(float delta_t) {
// //    A_d = (A * delta_t).exp();
//
//     // Because we know the A matrix we can calculate this ourselves
//     A_d = Eigen::Matrix<float, 3, 3>{{1, delta_t, 0.5f * std::pow(delta_t, 2.0f)},
//                                      {0, 1,       delta_t},
//                                      {0, 0,       1}
//     };
// }
//
// void AltitudeKf::predict(double current_time) {
//     // Calculate time since last loop
//     auto delta_t = std::min((float) (current_time - lastTime), 1.0f); // The clamping should only matter the first time through.  If you call this slower than once per second there will be a problem
//     lastTime = current_time;
//
//     // Discretize
//     calculateDiscreteTimeA(delta_t);
//
//     // Move state estimate forward in time
//     x = A_d * x;
//
//     // TODO: Move state covariance forward in time
// }
//
// void AltitudeKf::genericUpdate(const Eigen::Matrix<float, 3, 1> &measurement, const Eigen::Matrix<float, 3, 3> &C) {
//     Eigen::Matrix<float, 3, 1> innovation = measurement - C * x;  // Innovation is [pos; vel; accel]
//
//     // TODO: Calculate innovation covariance
//     // TODO: Calculate gain
//     auto L = Eigen::Matrix<float, 3, 3>{{0.1,  0, 0},  // Position
//                                         {0.05, 0, 0.05},  // Velocity
//                                         {0,    0, 1},  // Acceleration
//     };
//
//     // Calculate new state estimate
//     x = x + L * innovation;
//
//     // TODO: Update state covariance
// }
//
// void AltitudeKf::altitudeDataUpdate(float altitude) {
//     Eigen::Matrix<float, 3, 3> C;
//     C.setZero();
//     C.row(0) = C_baro;
//     genericUpdate({altitude, 0, 0}, C);
// }
//
// void AltitudeKf::accelerationDataUpdate(float acceleration) {
//     Eigen::Matrix<float, 3, 3> C;
//     C.setZero();
//     C.row(2) = C_accel;
//     genericUpdate({0, 0, acceleration}, C);
// }
//
// void AltitudeKf::dataUpdate(float altitude, float acceleration) {
//     Eigen::Matrix<float, 3, 3> C;
//     C.setZero();
//     C.row(0) = C_baro;
//     C.row(2) = C_accel;
//     genericUpdate({altitude, 0, acceleration}, C);
// }
//
// Eigen::Matrix<float, 3, 1> AltitudeKf::getStateVector() {
//     return x;
// }
//
// float AltitudeKf::getAltitude() {
//     return getStateVector()[0];
// }
