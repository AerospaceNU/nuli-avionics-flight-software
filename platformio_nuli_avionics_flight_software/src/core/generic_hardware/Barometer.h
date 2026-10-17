#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H

#include <Avionics.h>
#include "GenericSensor.h"
#include "ConstantsUnits.h"
#include <cmath>

/** @brief Barometer base class; also usable directly with inject() for externally supplied data (e.g. desktop sim). */
class Barometer : public GenericSensor {
public:
    /** @brief Sets the reading directly; it is valid only if the pressure is plausible. */
    void inject(const float temperatureK, const float humidityPercent, const float pressurePa) {
        m_temperatureK = temperatureK;
        m_humidityPercent = humidityPercent;
        m_pressurePa = pressurePa;
        m_validReading = isPlausiblePressure(pressurePa);
    }

    /** @brief Marks the current reading as not a measurement, until the next plausible inject(). */
    void invalidate() {
        m_validReading = false;
    }

    bool validReading() override {
        return m_validReading;
    }

    static bool isPlausiblePressure(const float pressurePa) {
        return std::isfinite(pressurePa) && pressurePa > 100.0f && pressurePa < 200000.0f;
    }

    float getTemperatureK() const {
        return m_temperatureK;
    }

    float getHumidityPercent() const {
        return m_humidityPercent;
    }

    float getPressurePa() const {
        return m_pressurePa;
    }

    /** @brief Standard-atmosphere altitude with temperatureK at sea level: lapse rate in the troposphere, isothermal above the tropopause. */
    static float calculateAltitudeM(const float pressurePa, const float temperatureK) {
        if (pressurePa >= TROPOPAUSE_PRESSURE_PA || pressurePa <= 0) return troposphereAltitudeM(pressurePa, temperatureK);
        const float tropopauseM = troposphereAltitudeM(TROPOPAUSE_PRESSURE_PA, temperatureK);
        const float tropopauseScaleHeightM = (float)(Constants::GAS_CONSTANT_J_KG_K / Constants::G_EARTH_MSS) * (temperatureK + (float)Constants::LAPSE_RATE_K_M * tropopauseM);
        return tropopauseM + tropopauseScaleHeightM * std::log(TROPOPAUSE_PRESSURE_PA / pressurePa);
    }

    /** @brief Absolute humidity in g/m^3; saturation vapour pressure from the Clausius-Clapeyron (Magnus) approximation. */
    static float calculateAbsoluteHumidity(const float temperatureK, const float humidityPercent) {
        // https://digital.library.unt.edu/ark:/67531/metadc693874/m1/15/ (eq. 25)

        const float saturationVapourPressure = 6.1094f * exp((17.625f * (temperatureK - Units::C_TO_K)) / ((temperatureK - Units::C_TO_K) + 243.04f)); // in hPa
        const float vapourPressure = (humidityPercent / 100) * saturationVapourPressure;

        return (216.7f * (vapourPressure)) / temperatureK;
    }

protected:
    static constexpr float TROPOPAUSE_PRESSURE_PA = 22632.1f;

    static float troposphereAltitudeM(const float pressurePa, const float temperatureK) {
        return (temperatureK / Constants::LAPSE_RATE_K_M) *
            (pow(pressurePa / Constants::ATMOSPHERIC_PRESSURE_PA, -Constants::GAS_CONSTANT_J_KG_K * Constants::LAPSE_RATE_K_M / Constants::G_EARTH_MSS) - 1);
    }

    float m_temperatureK = 0;
    float m_humidityPercent = 0; ///< relative humidity
    float m_pressurePa = 0;
    bool m_validReading = false;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BAROMETER_H
