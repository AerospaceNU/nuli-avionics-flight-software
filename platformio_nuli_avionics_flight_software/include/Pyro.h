#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_PYRO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_PYRO_H

#include <Avionics.h>
#include <GenericSensor.h>

/**
 * @class Pyro
 * @brief Represents a pyro channel
 * @details Ability to fire, reading continuity. This assumes the device is an arduino with a pin to read state and a pin to fire
 */
class Pyro : public GenericSensor {
public:
//    /**
//     * @brief Creates a pyro object
//     * @param firePin Digital pin to fire the charge
//     * @param continuityPin Pin to read in continuity
//     * @param continuityThreshold Analog threshold for determining continuity
//     */
//    Pyro(uint8_t firePin, uint8_t continuityPin, int32_t continuityThreshold) : m_firePin(firePin), m_continuityPin(continuityPin),
//                                                                                m_continuityThreshold(continuityThreshold) {}
//
//    /**
//     * @brief Initializes the pyro
//     * @details Sets up the input/output pins
//     */
//    void setup() override {
//        if (m_continuityThreshold == USE_DIGITAL_CONTINUITY) {
//            pinMode(m_continuityPin, INPUT);
//        }
//        pinMode(m_firePin, OUTPUT);
//        disable();
//    };
//
//    /**
//     * @brief Reads in the continuity state
//     * @details Can use either analog or digital input
//     */
//    void read() override {
//        if (m_continuityThreshold == USE_DIGITAL_CONTINUITY) {
//            m_hasContinuity = digitalRead(m_continuityPin);
//        } else {
//            m_hasContinuity = analogRead(m_continuityPin) >= uint32_t(m_continuityThreshold);
//        }
//    }
//
//    /**
//     * @brief Returns if the channel has pyro continuity
//     * @return If there is continuity
//     */
//    inline bool hasContinuity() const {
//        return m_hasContinuity;
//    }
//
//    /**
//     * @brief Fires the pyro channel
//     * @details Writes the fire pin high
//     */
//    inline void fire() const {
//        digitalWrite(m_firePin, HIGH);
//    }
//
//    /**
//     * @brief Disables they pyro channel
//     * @details Writes the fire pin low
//     */
//    inline void disable() const {
//        digitalWrite(m_firePin, LOW);
//    }
//
//    static constexpr int32_t USE_DIGITAL_CONTINUITY = -1;           ///< Flag value for the analog threshold to allow for continuity to be read digitally

private:
    bool m_hasContinuity = false;               ///< Tracks if the pyro has continuity
    const uint8_t m_firePin;                    ///< Pin for firing the pyro
    const uint8_t m_continuityPin;              ///< Pin for reading pyro continuity
    const int32_t m_continuityThreshold;        ///< Analog threshold for determining if a pin has continuity
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_PYRO_H
