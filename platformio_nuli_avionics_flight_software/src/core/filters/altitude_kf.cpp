#include "altitude_kf.h"

//#include "unsupported/Eigen/MatrixFunctions"
//#include ArduinoEigenDense

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
    A = Eigen::Matrix<float, 3, 3>{{0, 1, 0},
                                   {0, 0, 1},
                                   {0, 0, 0}
    };

    // Where we're going, we don't need a B matrix

    // Each sensor gets a different C matrix
    C_baro = {1, 0, 0};
    C_accel = {0, 0, 1};

    // As long as we measure position, (which we do), the system is observable.  Observability is so easy
}

void AltitudeKf::calculateDiscreteTimeA(float delta_t) {
//    A_d = (A * delta_t).exp();

    // Because we know the A matrix we can calculate this ourselves
    A_d = Eigen::Matrix<float, 3, 3>{{1, delta_t, 0.5f * std::pow(delta_t, 2.0f)},
                                     {0, 1,       delta_t},
                                     {0, 0,       1}
    };
}

void AltitudeKf::predict(double current_time) {
    // Calculate time since last loop
    auto delta_t = std::min((float) (current_time - lastTime), 1.0f); // The clamping should only matter the first time through.  If you call this slower than once per second there will be a problem
    lastTime = current_time;

    // Discretize
    calculateDiscreteTimeA(delta_t);

    // Move state estimate forward in time
    x = A_d * x;

    // TODO: Move state covariance forward in time
}

void AltitudeKf::genericUpdate(const Eigen::Matrix<float, 3, 1> &measurement, const Eigen::Matrix<float, 3, 3> &C) {
    Eigen::Matrix<float, 3, 1> innovation = measurement - C * x;  // Innovation is [pos; vel; accel]

    // TODO: Calculate innovation covariance
    // TODO: Calculate gain
    auto L = Eigen::Matrix<float, 3, 3>{{0.1,  0, 0},  // Position
                                        {0.05, 0, 0.05},  // Velocity
                                        {0,    0, 1},  // Acceleration
    };

    // Calculate new state estimate
    x = x + L * innovation;

    // TODO: Update state covariance
}

void AltitudeKf::altitudeDataUpdate(float altitude) {
    Eigen::Matrix<float, 3, 3> C;
    C.setZero();
    C.row(0) = C_baro;
    genericUpdate({altitude, 0, 0}, C);
}

void AltitudeKf::accelerationDataUpdate(float acceleration) {
    Eigen::Matrix<float, 3, 3> C;
    C.setZero();
    C.row(2) = C_accel;
    genericUpdate({0, 0, acceleration}, C);
}

void AltitudeKf::dataUpdate(float altitude, float acceleration) {
    Eigen::Matrix<float, 3, 3> C;
    C.setZero();
    C.row(0) = C_baro;
    C.row(2) = C_accel;
    genericUpdate({altitude, 0, acceleration}, C);
}

Eigen::Matrix<float, 3, 1> AltitudeKf::getStateVector() {
    return x;
}

float AltitudeKf::getAltitude() {
    return getStateVector()[0];
}
