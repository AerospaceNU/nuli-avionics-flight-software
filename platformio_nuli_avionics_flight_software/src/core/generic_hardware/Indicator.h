#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATOR_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATOR_H

#include "Avionics.h"

class Indicator {
public:
    virtual ~Indicator() = default;

    enum IndicatorType {
        NONE,
        AUDIO,
        VISUAL,
    };

    virtual void setup(DebugStream *debugStream) {}

    virtual void on() = 0;

    virtual void off() = 0;

    virtual void setPercent(float percent) = 0;

    IndicatorType getType() const {
        return m_type;
    }

protected:
    explicit Indicator(const IndicatorType type) : m_type(type) {};

    IndicatorType m_type = NONE;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_INDICATOR_H
