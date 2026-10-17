#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINO_PYRO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINO_PYRO_H

#include <Avionics.h>
#include "util/Timer.h"
#include "core/generic_hardware/GenericHardware.h"


/** @brief Pyro channel with a fire pin and an analog continuity input */
class ArduinoPyro final : public Pyro {
public:
    /**
     * @param continuityThreshold Analog threshold; negative means continuity when the reading is <= -threshold
     * @param armedThreshold Higher threshold separating "igniter detected" from "armed" (e.g. SeriousGoose); defaults to continuityThreshold
     */
    ArduinoPyro(const uint8_t firePin, const uint8_t continuityPin, const int32_t continuityThreshold, const int32_t armedThreshold = NO_SEPARATE_ARMED_THRESHOLD) :
            m_firePin(firePin), m_continuityPin(continuityPin), m_continuityThreshold(continuityThreshold),
            m_armedThreshold(armedThreshold == NO_SEPARATE_ARMED_THRESHOLD ? continuityThreshold : armedThreshold) {}

    void setup(DebugStream *debugStream, WatchdogTimer *watchdog) override;

    void read() override;

    void run() override;

    bool hasContinuity() const override;

    /** @brief Continuity and past the armed threshold */
    bool isArmed() const override;

    void fire() override;

    void fireFor(uint32_t timeMs) override;


    void disable() override;


    int rawAdcValue() const override;

    bool isFired() const override;


    static constexpr int32_t USE_DIGITAL_CONTINUITY = -1;           ///< Continuity threshold for a digital (active-low) continuity input
    static constexpr int32_t NO_SEPARATE_ARMED_THRESHOLD = INT32_MIN; ///< armedThreshold default: isArmed() mirrors hasContinuity()

private:
    bool m_isFired = false;
    bool m_hasContinuity = false;
    bool m_isArmed = false;
    int32_t m_continuityValue = 0;
    const uint8_t m_firePin;
    const uint8_t m_continuityPin;
    const int32_t m_continuityThreshold;
    const int32_t m_armedThreshold;

    Alarm m_timedFireAlarm;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINO_PYRO_H
