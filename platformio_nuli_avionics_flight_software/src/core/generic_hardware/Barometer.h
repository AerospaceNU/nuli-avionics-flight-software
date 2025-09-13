#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H

#include <Avionics.h>
#include "GenericSensor.h"
#include "ConstantsUnits.h"

/**
 * @class Barometer
 * @brief Generic Barometer
 * @details Base class for all barometers. Can also be used as an "injected" class, where an external source provides the data (i.e. desktop sim)
 */
class Barometer : public GenericSensor {
public:
    /**
     * @brief Injects sensor data directly
     * @details If a sensor can't be directly read from, you can inject data directly to the class
     * @param temperatureK Temperature in k
     * @param humidityPercent Humidity in percent
     * @param pressurePa Pressure in pascals
     */
    void inject(const float temperatureK, const float humidityPercent, const float pressurePa) {
        m_temperatureK = temperatureK;
        m_humidityPercent = humidityPercent;
        m_pressurePa = pressurePa;
    }

    /**
     * @brief Gets current temperature of the barometer
     * @return Temperature in k
     */
    float getTemperatureK() const {
        return m_temperatureK;
    }

    /**
     * @brief Gets the current humidity
     * @return Humidity in percent
     */
    float getHumidityPercent() const {
        return m_humidityPercent;
    }

    /**
     * @brief Gets the current pressure
     * @return Pressure in atmospheres
     */
    float getPressurePa() const {
        return m_pressurePa;
    }

protected:
    // /**
    //  * @brief Calculates the altitude from pressure and temperature
    //  * @details Specific algorithm used??????
    //  */
    //
    // /**
    //  * @brief Calculates the absolute humidity in g/m^3 from relative humidity and temperature
    //  * @details Used the Clausius–Clapeyron for the calculation of saturation vapour pressure. And
    //  */
    // void calculateAbsoluteHumidity() {
    //     // @todo use a constant for conversion to and from kelvin
    //     // https://www.omnicalculator.com/physics/absolute-humidity
    //     // comes from https://en.wikipedia.org/wiki/Clausius%E2%80%93Clapeyron_relation#Meteorology_and_climatology
    //     // direct source https://digital.library.unt.edu/ark:/67531/metadc693874/m1/15/ (eq. 25)
    //     // this is also an approximation
    //
    //     const double saturationVapourPressure = 6.1094 * exp((17.625 * (m_temperatureK - Units::C_TO_K)) / ((m_temperatureK - Units::C_TO_K) + 243.04)); // in hPa
    //     // equation can be seen from here (re-arrange RH) http://www.atmo.arizona.edu/students/courselinks/fall12/atmo336/lectures/sec1/humidity.html
    //     const double vapourPressure = (m_humidityPercent / 100) * saturationVapourPressure;
    //
    //     m_absoluteHumidity = (216.7 * (vapourPressure)) / m_temperatureK;
    // }

    float m_temperatureK = 0; ///<The measured temperature
    float m_humidityPercent = 0; ///< The measured humidity (%rh)
    float m_pressurePa = 0; ///< The measured pressure
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H
