#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H

#include <Avionics.h>
#include <GenericSensor.h>

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
    inline void inject(double temperatureK, double humidityPercent, double pressurePa) {
        m_temperatureK = temperatureK;
        m_humidityPercent = humidityPercent;
        m_pressurePa = pressurePa;
        calculateAltitude();
    }

    /**
     * @brief Gets current temperature of the barometer
     * @return Temperature in k
     */
    inline double getTemperatureK() const {
        return m_temperatureK;
    }

    /**
     * @brief Gets the current humidity
     * @return Humidity in percent
     */
    inline double getHumidityPercent() const {
        return m_humidityPercent;
    }

    /**
     * @brief Gets the absolute humidity
     * @return Humidity in g/m^3
     */
    inline double getAbsoluteHumidity() const {
        return m_absoluteHumidity;
    }

    /**
     * @brief Gets the current pressure
     * @return Pressure in atmospheres
     */
    inline double getPressurePa() const {
        return m_pressurePa;
    }

    /**
     * Gets the current altitude
     * @return Altitude in m
     */
    inline double getAltitudeM() const {
        return m_altitudeM;
    }

protected:
    /**
     * @brief Calculates the altitude from pressure and temperature
     * @details Specific algorithm used??????
     */
    void calculateAltitude() {
        // STM32 Calculation    @todo figure out most accurate calculation
        m_altitudeM = (m_temperatureK / LAPSE_RATE_K_M) *
                      (pow(m_pressurePa / ATMOSPHERIC_PRESSURE_PA, -GAS_CONSTANT_J_KG_K * LAPSE_RATE_K_M / GRAVITATIONAL_ACCELERATION_M_SS) - 1);
        // Chatgpt formula, confirmed here: https://www.mide.com/air-pressure-at-altitude-calculator
//        m_altitudeM = (m_temperatureK / LAPSE_RATE_K_M) *
//                      (1 - pow(m_pressurePa / ATMOSPHERIC_PRESSURE_PA, (GAS_CONSTANT_J_KG_K * LAPSE_RATE_K_M) / GRAVITATIONAL_ACCELERATION_M_SS));
    }

    /**
     * @brief Calculates the absolute humidity in g/m^3 from relative humidity and temperature
     * @details Used the Clausius–Clapeyron for the calculation of saturation vapour pressure. And
     */
    void calculateAbsoluteHumidity() {
        // @todo use a constant for conversion to and from kelvin
        // https://www.omnicalculator.com/physics/absolute-humidity
        // comes from https://en.wikipedia.org/wiki/Clausius%E2%80%93Clapeyron_relation#Meteorology_and_climatology
        // direct source https://digital.library.unt.edu/ark:/67531/metadc693874/m1/15/ (eq. 25)
        // this is also an approximation
        double saturationVapourPressure =  6.1094 * exp((17.625 * (m_temperatureK - 273.15)) / ((m_temperatureK - 273.15) + 243.04));   // in hPa
        // equation can be seen from here (re-arrange RH) http://www.atmo.arizona.edu/students/courselinks/fall12/atmo336/lectures/sec1/humidity.html
        double vapourPressure = (m_humidityPercent/100) * saturationVapourPressure;

        m_absoluteHumidity = (216.7 * (vapourPressure)) / m_temperatureK;
    }

    double m_temperatureK = 0;          ///<The measured temperature
    double m_humidityPercent = 0;       ///< The measured humidity (%rh)
    double m_absoluteHumidity = 0;      ///< The calculated absolute humidity
    double m_pressurePa = 0;            ///< The measured pressure
    double m_altitudeM = 0;             ///< The calculated altitude
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H
