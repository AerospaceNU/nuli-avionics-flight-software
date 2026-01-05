#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATORBUZZER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATORBUZZER_H

#include "Avionics.h"
#include <Arduino.h>
#include "../../core/generic_hardware/Indicator.h"

class IndicatorBuzzer final : public Indicator {
public:
    explicit IndicatorBuzzer(const int8_t pin, const uint32_t frequency, const uint32_t bandwidth)
        : Indicator(AUDIO), m_pin(pin), m_frequency(frequency), m_bandwidth(bandwidth) {}

    void on() override {
        if (!m_active && m_enabled) {
            m_active = true;
            tone(m_pin, m_frequency);
        }
    }

    void off() override {
        if (m_active) {
            noTone(m_pin);
            m_active = false;
        }
    }

    void setPercent(const float percent) override {
        if (!m_enabled) return;  // Block if disabled

        const float normalized = constrain(percent, 0.0f, 100.0f) / 100.0f;
        const float offset = (normalized - 0.5f) * float(m_bandwidth);
        const uint32_t freq = uint32_t(float(m_frequency) + offset);
        tone(m_pin, freq);
        m_active = true;
    }

    void disable() {
        m_enabled = false;
        off();  // Ensure buzzer is off immediately
    }

    void enable() {
        m_enabled = true;
    }

private:
    int8_t m_pin;
    uint32_t m_frequency;
    uint32_t m_bandwidth;
    bool m_active = false;
    bool m_enabled = true;  // Added: tracks whether buzzer is enabled
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATORBUZZER_H
