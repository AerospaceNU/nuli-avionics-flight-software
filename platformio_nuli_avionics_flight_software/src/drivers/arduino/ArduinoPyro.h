#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINO_PYRO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINO_PYRO_H

#include <Avionics.h>
#include "../../core/generic_hardware/GenericSensor.h"
#include "../../core/generic_hardware/Pyro.h"


/**
 * @class Pyro
 * @brief Represents a pyro channel
 * @details Ability to fire, reading continuity. This assumes the device is an arduino with a pin to read state and a pin to fireDrogue
 */
class ArduinoPyro : public Pyro {
public:
    /**
     * @brief Creates a pyro object
     * @param firePin Digital pin to fireDrogue the charge
     * @param continuityPin Pin to read in continuity
     * @param continuityThreshold Analog threshold for determining continuity
     */
    ArduinoPyro(uint8_t firePin, uint8_t continuityPin, int32_t continuityThreshold) :
            m_firePin(firePin), m_continuityPin(continuityPin), m_continuityThreshold(continuityThreshold) {}

    /**
     * @brief Initializes the pyro
     * @details Sets up the input/output pins
     */
    void setup() override;

    /**
     * @brief Reads in the continuity state
     * @details Can use either analog or digital input
     */
    void read() override;
    /**
     * @brief Returns if the channel has pyro continuity
     * @return If there is continuity
     */
    bool hasContinuity() const override;
    /**
     * @brief Fires the pyro channel
     * @details Writes the fireDrogue pin high
     */
    void fire() override;
    /**
     * @brief Disables they pyro channel
     * @details Writes the fireDrogue pin low
     */
    void disable() override;


    int rawAdcValue() const override;

    bool isFired() const override;


    static constexpr int32_t USE_DIGITAL_CONTINUITY = -1;           ///< Flag value for the analog threshold to allow for continuity to be read digitally

private:
    bool m_isFired = false;
    bool m_hasContinuity = false;               ///< Tracks if the pyro has continuity
    int32_t m_continuityValue = 0;              ///< Analog threshold for determining if a pin has continuity
    const uint8_t m_firePin;                    ///< Pin for firing the pyro
    const uint8_t m_continuityPin;              ///< Pin for reading pyro continuity
    const int32_t m_continuityThreshold;        ///< Analog threshold for determining if a pin has continuity
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINO_PYRO_H
