#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H

#include <Avionics.h>
#include <BaseSensor.h>

class Barometer : public BaseSensor {
public:
    void inject(double temperatureC = 0, double humidity = 0, double pressureAtm = 0) {
        m_temperatureC = temperatureC;
        m_humidity = humidity;
        m_pressureAtm = pressureAtm;
        calculateAltitude();
    }

protected:
    void calculateAltitude() {
//        (tempRef / lapseRate) *
//        (pow(presAvg / presRef, -R_DRY_AIR * lapseRate / G_ACCEL_EARTH) - 1);
    }

    double m_temperatureC = 0;
    double m_humidity = 0;
    double m_pressureAtm = 0;
    double m_altitudeM = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H
