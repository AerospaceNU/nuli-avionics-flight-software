#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_PYRO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_PYRO_H

#include <Avionics.h>
#include "GenericSensor.h"

/**
 * @class Pyro
 * @brief Represents a pyro channel
 * @details Ability to fire, reading continuity. This assumes the device is an arduino with a pin to read state and a pin to fireDrogue
 */
class Pyro : public GenericSensor {
public:
    /**
     * @brief Initializes the pyro
     * @details Sets up the input/output pins
     */
    void setup(DebugStream* debugStream, WatchdogTimer* watchdog) override {}

    /**
     * @brief Reads in the continuity state
     * @details Can use either analog or digital input
     */
    void read() override {}

    void run() override {};

    /**
     * @brief Returns if the channel has pyro continuity
     * @return If there is continuity
     */
    virtual bool hasContinuity() const { return false; }

    /**
     * @brief Returns if the channel has continuity AND is armed
     * @details Boards with only a single continuity threshold (e.g. SillyGoose) have no separate
     * notion of "armed" - for them this defaults to just mirroring hasContinuity(), so continuity
     * alone reads as armed. Boards with a second, higher threshold (e.g. SeriousGoose) override
     * this to distinguish "igniter detected but not armed" from "armed and ready to fire".
     * @return If the channel is armed
     */
    virtual bool isArmed() const { return hasContinuity(); }

    /**
     * @brief Packs this channel's continuity/armed reading into a single 0/1/2 state byte, for
     * logging/telemetry formats that store one byte per pyro channel (0 = no continuity, 1 =
     * continuity but not armed, 2 = armed).
     */
    uint8_t stateByte() const { return isArmed() ? 2 : (hasContinuity() ? 1 : 0); }


    /**
     * @brief Returns if the channel is currently fired
     * @return If fired
     */
    virtual bool isFired() const { return false; }


    /**
     * @brief Fires the pyro channel
     * @details Writes the fireDrogue pin high
     */
    virtual void fire() {}

    virtual void fireFor(uint32_t timeMs) {}

    /**
     * @brief Disables they pyro channel
     * @details Writes the fireDrogue pin low
     */
    virtual void disable() {}

    virtual int rawAdcValue() const { return 0; };

protected:
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_PYRO_H
