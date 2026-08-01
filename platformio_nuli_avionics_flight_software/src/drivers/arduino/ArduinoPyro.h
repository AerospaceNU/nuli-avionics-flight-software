#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINO_PYRO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINO_PYRO_H

#include <Avionics.h>
#include "util/Timer.h"
#include "core/generic_hardware/GenericHardware.h"


/**
 * @class Pyro
 * @brief Represents a pyro channel
 * @details Ability to fire, reading continuity. This assumes the device is an arduino with a pin to read state and a pin to fireDrogue
 */
class ArduinoPyro final : public Pyro {
public:
    /**
     * @brief Creates a pyro object
     * @param firePin Digital pin to fireDrogue the charge
     * @param continuityPin Pin to read in continuity
     * @param continuityThreshold Analog threshold for determining continuity
     * @param armedThreshold Optional second, higher analog threshold for boards that can
     * distinguish "igniter detected but not armed" from "armed and ready to fire" (e.g.
     * SeriousGoose). Defaults to NO_SEPARATE_ARMED_THRESHOLD, which makes isArmed() just mirror
     * hasContinuity() - correct for boards with only one threshold (e.g. SillyGoose).
     */
    ArduinoPyro(const uint8_t firePin, const uint8_t continuityPin, const int32_t continuityThreshold, const int32_t armedThreshold = NO_SEPARATE_ARMED_THRESHOLD) :
            m_firePin(firePin), m_continuityPin(continuityPin), m_continuityThreshold(continuityThreshold),
            m_armedThreshold(armedThreshold == NO_SEPARATE_ARMED_THRESHOLD ? continuityThreshold : armedThreshold) {}

    /**
     * @brief Initializes the pyro
     * @details Sets up the input/output pins
     */
    void setup(DebugStream *debugStream, WatchdogTimer *watchdog) override;

    /**
     * @brief Reads in the continuity state
     * @details Can use either analog or digital input
     */
    void read() override;

    void run() override;

    /**
     * @brief Returns if the channel has pyro continuity
     * @return If there is continuity
     */
    bool hasContinuity() const override;

    /**
     * @brief Returns if the channel has continuity AND has crossed the (higher) armed threshold
     * @return If the channel is armed
     */
    bool isArmed() const override;

    /**
     * @brief Fires the pyro channel
     * @details Writes the fireDrogue pin high
     */
    void fire() override;

    void fireFor(uint32_t timeMs) override;


    /**
     * @brief Disables they pyro channel
     * @details Writes the fireDrogue pin low
     */
    void disable() override;


    int rawAdcValue() const override;

    bool isFired() const override;


    static constexpr int32_t USE_DIGITAL_CONTINUITY = -1;           ///< Flag value for the analog threshold to allow for continuity to be read digitally
    static constexpr int32_t NO_SEPARATE_ARMED_THRESHOLD = INT32_MIN; ///< Sentinel default for armedThreshold - see constructor doc

private:
    bool m_isFired = false;
    bool m_hasContinuity = false;               ///< Tracks if the pyro has continuity
    bool m_isArmed = false;                     ///< Tracks if the pyro is armed (continuity AND past m_armedThreshold)
    int32_t m_continuityValue = 0;              ///< Analog threshold for determining if a pin has continuity
    const uint8_t m_firePin;                    ///< Pin for firing the pyro
    const uint8_t m_continuityPin;              ///< Pin for reading pyro continuity
    const int32_t m_continuityThreshold;        ///< Analog threshold for determining if a pin has continuity
    const int32_t m_armedThreshold;             ///< Analog threshold for determining if a pin is armed (== m_continuityThreshold on single-threshold boards)

    Alarm m_timedFireAlarm;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINO_PYRO_H
