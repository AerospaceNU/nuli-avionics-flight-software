#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_PYRO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_PYRO_H

#include <Avionics.h>
#include "GenericSensor.h"

/** @brief Pyro channel: continuity/armed sensing and firing. The base implementation is a no-op. */
class Pyro : public GenericSensor {
public:
    void setup(DebugStream* debugStream, WatchdogTimer* watchdog) override {}

    void read() override {}

    void run() override {};

    virtual bool hasContinuity() const { return false; }

    /** @brief Continuity AND armed. Defaults to hasContinuity() for single-threshold boards (SillyGoose); SeriousGoose overrides it. */
    virtual bool isArmed() const { return hasContinuity(); }

    /** @brief Log/telemetry state byte: 0 = no continuity, 1 = continuity but not armed, 2 = armed. */
    uint8_t stateByte() const { return isArmed() ? 2 : (hasContinuity() ? 1 : 0); }


    virtual bool isFired() const { return false; }


    virtual void fire() {}

    virtual void fireFor(uint32_t timeMs) {}

    virtual void disable() {}

    virtual int rawAdcValue() const { return 0; };

protected:
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_PYRO_H
