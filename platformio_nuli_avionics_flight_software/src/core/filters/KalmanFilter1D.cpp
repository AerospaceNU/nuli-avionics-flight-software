#include "KalmanFilter1D.h"

KalmanFilter1D::KalmanFilter1D() {
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

void KalmanFilter1D::setDeltaTime(float delta_t) {
    // Can't use matrix exponential on embedded systems, because that's not in ArduinoEigen
    // Instead, we calculate this ourselves
    F_d = Eigen::Matrix<float, 3, 3>{{1, delta_t, 0.5f * std::pow(delta_t, 2.0f)},
                                     {0, 1,       delta_t},
                                     {0, 0,       1}
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
    // Calculate time since last loop
    auto delta_t = std::min((float) (current_time - lastTime), 1.0f); // The clamping should only matter the first time through.  If you call this slower than once per second there will be a problem
    lastTime = current_time;

    // Discretize
    setDeltaTime(delta_t);

    // Predict
    predict();
}

void KalmanFilter1D::predict() {
    // Move state estimate forward in time
    x = F_d * x;

    // Move state covariance forward in time
    P = F_d * P * F_d.transpose() + Q;
}

void KalmanFilter1D::genericUpdate(const Eigen::Matrix<float, 3, 1> &measurement, const Eigen::Matrix<float, 3, 1> &mask) {
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
