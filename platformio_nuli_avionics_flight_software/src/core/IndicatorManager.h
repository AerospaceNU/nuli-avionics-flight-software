#ifndef INDICATORMANAGER_H
#define INDICATORMANAGER_H

#include "Avionics.h"
#include "HardwareAbstraction.h"
#include "core/generic_hardware/Indicator.h"
#include "core/generic_hardware/Pyro.h"

class IndicatorManager {
public:
    void setup(HardwareAbstraction* hardwareAbstraction, const int16_t drogueID, const int16_t mainID) {
        m_hardware = hardwareAbstraction;
        m_drogue = m_hardware->getPyro(drogueID);
        m_main = m_hardware->getPyro(mainID);
    }

    void beepContinuity(const Timestamp_s& timestamp) const {
        const bool drogueContinuity = m_drogue->hasContinuity();
        const bool mainContinuity = m_main->hasContinuity();
        int numBeeps = 0;
        if (drogueContinuity && mainContinuity) {
            numBeeps = 3;
        } else if (mainContinuity) {
            numBeeps = 2;
        } else if (drogueContinuity) {
            numBeeps = 1;
        } else {
            numBeeps = 0;
        }

        bool active = false;

        if (numBeeps > 0) {
            // === Normal continuity beep sequence ===
            constexpr uint32_t beepOnMs = 100; // short beep
            constexpr uint32_t beepOffMs = 100; // gap between beeps
            constexpr uint32_t cycleGapMs = 800; // gap after full sequence

            const uint32_t sequenceDuration = numBeeps * (beepOnMs + beepOffMs) + cycleGapMs;
            const uint32_t t = timestamp.runtime_ms % sequenceDuration;

            for (int b = 0; b < numBeeps; b++) {
                const uint32_t start = b * (beepOnMs + beepOffMs);
                const uint32_t end = start + beepOnMs;
                if (t >= start && t < end) {
                    active = true;
                    break;
                }
            }
        } else {
            // === No continuity: long beep, short pause ===
            constexpr uint32_t longBeepMs = 1000;
            constexpr uint32_t shortPauseMs = 2000;
            constexpr uint32_t sequenceDuration = longBeepMs + shortPauseMs;
            const uint32_t t = timestamp.runtime_ms % sequenceDuration;

            if (t < longBeepMs) {
                active = true;
            } else {
                active = false;
            }
        }

        // Apply to all indicators
        for (int i = 0; i < m_hardware->getNumIndicators(); i++) {
            if (active) {
                m_hardware->getIndicator(i)->on();
            } else {
                m_hardware->getIndicator(i)->off();
            }
        }
    }


    void keepAliveBeep(const Timestamp_s& timestamp) const {
        for (int i = 0; i < m_hardware->getNumIndicators(); i++) {
            if ((timestamp.runtime_ms / 200) % 2 == 0) {
                m_hardware->getIndicator(i)->on();
            } else {
                m_hardware->getIndicator(i)->off();
            }
        }
    }

    void siren(const Timestamp_s& timestamp) const {
        // Adjust this to control how fast the siren oscillates
        constexpr float period_ms = 1000.0f; // one full cycle every 2 seconds

        // Convert runtime to a phase in radians [0, 2π]
        const float phase = (timestamp.runtime_ms % (uint32_t)period_ms) * (2.0f * M_PI / period_ms);

        // Sinusoidal value from 0 to 100
        const float percent = (sinf(phase) * 0.5f + 0.5f) * 100.0f;

        for (int i = 0; i < m_hardware->getNumIndicators(); i++) {
            m_hardware->getIndicator(i)->setPercent(percent);
        }
    }

private:
    HardwareAbstraction* m_hardware = nullptr;
    Pyro* m_drogue = nullptr;
    Pyro* m_main = nullptr;
};

#endif //INDICATORMANAGER_H
