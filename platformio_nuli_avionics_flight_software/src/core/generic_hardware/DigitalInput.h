#ifndef DIGITALPIN_H
#define DIGITALPIN_H

#include "Avionics.h"
#include "core/generic_hardware/GenericHardware.h"

class DigitalInput : public GenericSensor {
public:
    bool isHigh() const {
        return m_high;
    }

    bool isLow() const {
        return !m_high;
    }

protected:
    bool m_high = false;
};

#endif //DIGITALPIN_H
