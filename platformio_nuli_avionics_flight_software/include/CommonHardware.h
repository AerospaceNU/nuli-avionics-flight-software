#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_COMMONHARDWARE_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_COMMONHARDWARE_H

#include <stdint.h>
#include <Arduino.h>

class MessageInterface {

};

class RadioTransmitter : public MessageInterface {
};

class SerialConnection : public MessageInterface {

};


class GPS {

};

class FlashMemory {

};

class Magnetometer {

};

class Accelerometer {

};

class Gyroscope {

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

class Barometer {
public:
    Barometer() = default;

    virtual void read() {

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

class Pyro {
public:
    Pyro(uint8_t firePin, uint8_t continuityPin, int32_t continuityThreshold) : m_firePin(firePin), m_continuityPin(continuityPin),
                                                                                m_continuityThreshold(continuityThreshold) {
        if (m_continuityThreshold == USE_DIGITAL_CONTINUITY) {
            pinMode(continuityPin, INPUT);
        }
        pinMode(firePin, OUTPUT);
        disable();
    }

    void read() {
        if (m_continuityThreshold == USE_DIGITAL_CONTINUITY) {
            m_hasContinuity = digitalRead(m_continuityPin);
        } else {
            m_hasContinuity = analogRead(m_continuityPin) >= m_continuityThreshold;
        }
    }

    bool hasContinuity() const {
        return m_hasContinuity;
    }

    void fire() const {
        digitalWrite(m_firePin, HIGH);
    }

    void disable() const {
        digitalWrite(m_firePin, LOW);
    }

    static constexpr int32_t USE_DIGITAL_CONTINUITY = -1;

private:
    bool m_hasContinuity = false;
    const uint8_t m_firePin;
    const uint8_t m_continuityPin;
    const int32_t m_continuityThreshold;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_COMMONHARDWARE_H
