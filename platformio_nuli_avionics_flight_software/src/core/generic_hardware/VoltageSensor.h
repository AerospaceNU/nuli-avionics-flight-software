#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_VOLTAGESENSOR_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_VOLTAGESENSOR_H

#include "Avionics.h"
#include "GenericSensor.h"

class VoltageSensor : public GenericSensor {
public:
    explicit VoltageSensor(const float scaleFactor) {
        m_scaleFactor = scaleFactor;
    }

    void setup() override {}

    void read() override {}

    void setScaleFactor(const float scaleFactor) {
        m_scaleFactor = scaleFactor;
    }

    void inject(const float rawVoltage) {
        m_rawVoltage = rawVoltage;
        scale();
    }

    virtual float getVoltage() { return m_voltage; }

    virtual float getRawVoltage() { return m_rawVoltage; }

protected:
    void scale() {
        m_voltage = m_rawVoltage * m_scaleFactor;
    }

    float m_scaleFactor = 1;
    float m_voltage = 0;
    float m_rawVoltage = 0;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_VOLTAGESENSOR_H
