#include "Avionics.h"
#include "GenericSensor.h"

#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_VOLTAGESENSOR_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_VOLTAGESENSOR_H

class VoltageSensor : public GenericSensor {
public:
    explicit VoltageSensor(double scaleFactor) {
        m_scaleFactor = scaleFactor;
    }

    void setup() override {}

    void read() override {}

    void inject(double rawVoltage) {
        m_rawVoltage = rawVoltage;
        scale();
    }

    virtual double getVoltage() { return m_voltage; }

    virtual double getRawVoltage() { return m_rawVoltage; }

protected:
    void scale() {
        m_voltage = m_rawVoltage * m_scaleFactor;
    }

    double m_scaleFactor = 1;
    double m_voltage = 0;
    double m_rawVoltage = 0;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_VOLTAGESENSOR_H
