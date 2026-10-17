#ifndef TIMER_H
#define TIMER_H

#include <cstdint>

/**
 * @brief True only once a condition has held continuously for the debounce time.
 */
class Debounce {
public:
    explicit Debounce(const uint32_t debounceTimeMs) : m_debounceTimeMs(debounceTimeMs) {}

    Debounce(const Debounce& other) = default;

    void setDebounceTime(const uint32_t debounceTimeMs) {
        m_debounceTimeMs = debounceTimeMs;
    }

    /**
     * @brief True if condition has been true continuously for the debounce time; false restarts the timer.
     */
    bool check(const bool condition, const uint32_t runtime_ms) {
        if (!m_isInitialized) {
            m_referenceTime = runtime_ms;
            m_isInitialized = true;
        }
        if (condition) {
            if (runtime_ms - m_referenceTime >= m_debounceTimeMs) {
                return true;
            }
        } else {
            m_referenceTime = runtime_ms;
        }
        return false;
    }

    /**
     * @brief The next check() starts timing from scratch.
     */
    void reset() {
        m_isInitialized = false;
    }

private:
    uint32_t m_debounceTimeMs = 0;
    uint32_t m_referenceTime = 0;
    bool m_isInitialized = false;
};


class Alarm {
public:
    void startAlarm(const uint32_t startTime, const uint32_t durationTime) {
        m_startTime = startTime;
        m_durationTime = durationTime;
        m_isInitialized = true;
    }

    bool isAlarmFinished(const uint32_t currentTime) const {
        if (!m_isInitialized) return false;
        return currentTime >= m_startTime + m_durationTime;
    }

    uint32_t getTimeElapsed(const uint32_t currentTime) const {
        if (!m_isInitialized) return false;
        return currentTime - m_startTime;
    }

    uint32_t timeRemaining(const uint32_t currentTime) const {
        if (!m_isInitialized) return 0;
        const uint32_t elapsed = getTimeElapsed(currentTime);
        return (elapsed >= m_durationTime) ? 0 : (m_durationTime - elapsed);
    }

    void reset() {
        m_isInitialized = false;
    }

    bool isInitialized() const {
        return m_isInitialized;
    }

private:
    uint32_t m_startTime = 0;
    uint32_t m_durationTime = 0;
    bool m_isInitialized = false;
};


class StopWatch {
public:
    void startWatch(const uint32_t startTime) {
        m_startTime = startTime;
        m_stoppedTimeElapsed = 0;
    }

    uint32_t getTimeElapsed(const uint32_t currentTime) const {
        return currentTime - m_startTime;
    }

    uint32_t getTimeUntil(const uint32_t currentTime, const uint32_t timeFromStart) const {
        const uint32_t targetTime = m_startTime + timeFromStart;
        return (currentTime >= targetTime) ? 0 : (targetTime - currentTime);
    }

    uint32_t stopWatch(const uint32_t currentTime) {
        m_stoppedTimeElapsed = getTimeElapsed(currentTime);
        return m_stoppedTimeElapsed;
    }

private:
    uint32_t m_startTime = 0;
    uint32_t m_stoppedTimeElapsed = 0;
};

#endif //TIMER_H
