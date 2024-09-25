#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H

#include <Avionics.h>
#include <BaseSensor.h>

class Barometer : public BaseSensor {
public:
    void inject(double temperatureC = 0, double humidity = 0, double pressureAtm = 0) {
        m_temperatureC = temperatureC;
        m_humidityPercent = humidity;
        m_pressureAtm = pressureAtm;
        calculateAltitude();
    }

    inline double getTemperatureC() const {
        return m_temperatureC;
    }

    inline double getHumidityPercent() const {
        return m_humidityPercent;
    }

    inline double getPressureAtm() const {
        return m_pressureAtm;
    }

    inline double getAltitudeM() const {
        return m_altitudeM;
    }

protected:
    void calculateAltitude() {
        // @todo add
//        (tempRef / lapseRate) *
//        (pow(presAvg / presRef, -R_DRY_AIR * lapseRate / G_ACCEL_EARTH) - 1);
    }

    double m_temperatureC = 0;
    double m_humidityPercent = 0;
    double m_pressureAtm = 0;
    double m_altitudeM = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H
