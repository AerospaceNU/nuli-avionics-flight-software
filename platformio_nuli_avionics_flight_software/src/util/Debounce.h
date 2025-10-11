#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include "Avionics.h"

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
     * @param timestamp The current timestamp as a Timestamp_s object, where runtime_ms represents the elapsed time in milliseconds.
     * @return true if the condition has been true for the debounce duration, false otherwise.
     */
    bool check(const bool condition, const Timestamp_s& timestamp) {
        if (!m_isInitialized) {
            m_referenceTime = timestamp.runtime_ms;
            m_isInitialized = true;
        }
        if (condition) {
            if (timestamp.runtime_ms - m_referenceTime >= m_debounceTimeMs) {
                return true;
            }
        } else {
            m_referenceTime = timestamp.runtime_ms;
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
    uint32_t m_debounceTimeMs; ///< Time in milliseconds for which the condition must remain true to be considered valid.
    uint32_t m_referenceTime = 0; ///< Reference timestamp used for debouncing.
    bool m_isInitialized = false; ///< Flag indicating whether the debounce object has been initialized.
};

#endif //DEBOUNCE_H