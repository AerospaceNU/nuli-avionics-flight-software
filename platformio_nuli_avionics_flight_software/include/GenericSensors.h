#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GENERICSENSORS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GENERICSENSORS_H

class Sensor {
public:
    virtual void read() {}

    virtual void setup() {}
};

class Barometer : public Sensor {
public:
    void read() override {

    }

protected:
    void calculateAltitude() {

    }

private:
    double m_temperatureC = 0;
    double m_humidity = 0;
    double m_altitudeM = 0;
    double m_pressureAtm = 0;
};


class Accelerometer : public Sensor {
public:
    void read() override {

    }
};

class Gyroscope : public Sensor {
public:
    void read() override {

    }
};

class Magnetometer : public Sensor {
public:
    void read() override {

    }
};

class GPS : public Sensor {
public:
    void read() override {

    }
};

class NineAxisIMU {
public:
    Gyroscope* getGyroscope() {
        return &m_gyroscope;
    }

    Magnetometer* getMagnetometer() {
        return &m_magnetometer;
    }

    Accelerometer* getAccelerometer() {
        return &m_accelerometer;
    }

private:
    Gyroscope m_gyroscope;
    Accelerometer m_accelerometer;
    Magnetometer m_magnetometer;
};

class SixAxis {

};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GENERICSENSORS_H
