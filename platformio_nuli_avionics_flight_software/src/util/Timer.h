#ifndef TIMER_H
#define TIMER_H

/**
 * @class Debounce
 * @brief Utility class to debounce boolean signals.
 * @details
 * This class provides a mechanism to debounce signals, typically used to prevent
 * spurious transitions in digital inputs due to noise or mechanical switch bounce.
 * It ensures that a condition must remain true for a specified amount of time
 * before it is considered valid.
 */
class Debounce {
public:
    /**
     * @brief Construct a new Debounce object with a specified debounce time.
     * @param debounceTimeMs The debounce time in milliseconds. The condition must
     *                       remain true for at least this duration before being
     *                       considered valid.
     */
    explicit Debounce(const uint32_t debounceTimeMs) : m_debounceTimeMs(debounceTimeMs) {}

    /**
     * @brief Copy constructor.
     * @param other The Debounce object to copy.
     */
    Debounce(const Debounce& other) = default;

    /**
     * @brief Set the debounce time.
     * @param debounceTimeMs The new debounce time in milliseconds.
     */
    void setDebounceTime(const uint32_t debounceTimeMs) {
        m_debounceTimeMs = debounceTimeMs;
    }

    /**
     * @brief Check if a condition has been stable for the debounce time.
     * @details When called with a boolean condition and the current timestamp, this
     * function returns true only if the condition has been continuously true
     * for at least the debounce time specified during construction or via
     * setDebounceTime(). If the condition is false, the internal reference
     * time is reset.
     * @param condition The boolean condition to debounce.
     * @param runtime_ms The current timestamp, where runtime_ms represents the system elapsed time in milliseconds
     * @return true if the condition has been true for the debounce duration, false otherwise.
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
     * @brief Reset the debounce state.
     * @details This function clears the internal state, including the reference time and
     * initialization flag. After calling reset(), the next call to check() will
     * treat the condition as if it were the first time being checked.
     */
    void reset() {
        m_isInitialized = false;
    }

private:
    uint32_t m_debounceTimeMs = 0; ///< Time in milliseconds for which the condition must remain true to be considered valid.
    uint32_t m_referenceTime = 0; ///< Reference timestamp used for debouncing.
    bool m_isInitialized = false; ///< Flag indicating whether the debounce object has been initialized.
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
    bool m_isInitialized = false; ///< Flag indicating whether the debounce object has been initialized.
};


class StopWatch {
public:
    void startWatch(const uint32_t startTime) {
        m_startTime = startTime;
        m_stoppedTimeElapsed = 0; // Reset if restarted
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
