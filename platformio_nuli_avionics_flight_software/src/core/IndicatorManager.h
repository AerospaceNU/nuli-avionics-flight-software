#ifndef INDICATORMANAGER_H
#define INDICATORMANAGER_H

#include "Avionics.h"
#include "HardwareAbstraction.h"
#include "core/generic_hardware/GenericHardware.h"

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

void configureBeepNums(const Timestamp_s& timestamp, const std::initializer_list<uint32_t> nums) {
        NumBeep &m_nbs = m_numBeepState;
        uint32_t time = 0;
        m_nbs.timeLast = timestamp.runtime_ms;
        m_nbs.timeDelta = 0;
        m_nbs.segmentCurrent = 0;
        m_nbs.segmentCount = 0;
		const auto addSegment = [&](const uint32_t delta, const NumBeep::SegmentType type) {
            m_nbs.timeSegments[m_nbs.segmentCount] = time += delta + NumBeep::DIGIT_SPACING_MS;
            m_nbs.typeSegments[m_nbs.segmentCount] = type;
            ++m_nbs.segmentCount;
        };
		for (uint32_t num : nums) {
            uint8_t digitCount = 0, digits[m_nbs.MAX_DIGITS];
			while (num) {
                digits[digitCount++] = num % 10;
                num /= 10;
			}
            if (m_nbs.segmentCount == 0)
                addSegment(m_nbs.SIREN_MS, NumBeep::SegmentType::SIREN);
            else
                addSegment(m_nbs.LONG_SPACING_MS, NumBeep::SegmentType::SPACING);
			while (digitCount--) {
                uint8_t digit = digits[digitCount];
                if (digit == 0) digit = 10;
                addSegment(m_nbs.DIGIT_HOLD_MS * 2 * digit, NumBeep::SegmentType::DIGIT);
			}
		}
    }

    void beepNums(const Timestamp_s &timestamp) {
        NumBeep &m_nbs = m_numBeepState;
        m_nbs.timeDelta += timestamp.runtime_ms - m_nbs.timeLast;
        m_nbs.timeLast = timestamp.runtime_ms;
        int32_t timeToNext;
		for (;;) {
            timeToNext = m_nbs.timeSegments[m_nbs.segmentCurrent] - m_nbs.timeDelta;
			if (timeToNext > 0) break;
            if (++m_nbs.segmentCurrent == m_nbs.segmentCount) {
                // Looped around: reset segment to start and wrap delta
                m_nbs.segmentCurrent = 0;
                m_nbs.timeDelta = -timeToNext;
			}
		}
        timeToNext -= m_nbs.DIGIT_SPACING_MS + 1;
        // NOTE: should really make it simpler to on/off every indicator
        // (should expose etl::vector IMO)
		if (timeToNext >= 0) {
			switch (m_nbs.typeSegments[m_nbs.segmentCurrent]) {
            case NumBeep::SegmentType::DIGIT:
				if (timeToNext % (m_nbs.DIGIT_HOLD_MS * 2) < m_nbs.DIGIT_HOLD_MS)
                    goto beepOff;
				[[fallthrough]];
            case NumBeep::SegmentType::SPACING:
                for (int i = 0; i < m_hardware->getNumIndicators(); i++)
                    m_hardware->getIndicator(i)->on();
                break;
            case NumBeep::SegmentType::SIREN:
                siren(timestamp);
                break;
            }
		} else {
        beepOff:
            for (int i = 0; i < m_hardware->getNumIndicators(); i++)
                m_hardware->getIndicator(i)->off();
		}
	}

  private:
    HardwareAbstraction *m_hardware = nullptr;
    Pyro* m_drogue = nullptr;
    Pyro* m_main = nullptr;

    struct NumBeep {
        enum class SegmentType : uint8_t {
            DIGIT, // Digit here
            SPACING, // Spacing between numbers here
            SIREN, // Initial siren here
        };

        static constexpr uint32_t MAX_DIGITS = 10;
        static constexpr uint32_t MAX_SEGMENT_COUNT = (MAX_DIGITS + 1) * MAX_INDICATOR_NUM;
        static constexpr uint32_t DIGIT_HOLD_MS = 200;
        static constexpr uint32_t DIGIT_SPACING_MS = 500;
        static constexpr uint32_t LONG_SPACING_MS = 1000;
        static constexpr uint32_t SIREN_MS = 2000;

        uint32_t timeLast = 0;
        uint32_t timeDelta = 0;
        uint32_t segmentCurrent = 0;
        uint32_t segmentCount = 0;
        uint32_t timeSegments[MAX_SEGMENT_COUNT] = {};
        SegmentType typeSegments[MAX_SEGMENT_COUNT] = {};
    };

    NumBeep m_numBeepState;
};

#endif //INDICATORMANAGER_H
