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
    void setup(DebugStream *debugStream) override {}

    /**
     * @brief Reads in the continuity state
     * @details Can use either analog or digital input
     */
    void read() override {}

    /**
     * @brief Returns if the channel has pyro continuity
     * @return If there is continuity
     */
    virtual bool hasContinuity() const { return false; }


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

    /**
     * @brief Disables they pyro channel
     * @details Writes the fireDrogue pin low
     */
    virtual void disable() {}

    virtual int rawAdcValue() const { return 0; };

protected:
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_PYRO_H
