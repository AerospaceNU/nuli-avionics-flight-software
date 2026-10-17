#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GENERICSENSOR_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GENERICSENSOR_H

#include <Avionics.h>
#include "DebugStream.h"
#include "GenericAvionicsHardware.h"

/** @brief Base for all sensors. */
class GenericSensor : public GenericAvionicsHardware {
public:
    virtual ~GenericSensor() = default;
    void setup(DebugStream *debugStream, WatchdogTimer *watchdog) override {}

    /** @brief Takes one reading; may block a few ms waiting on the sensor. */
    void read() override {}


    virtual bool validReading() { return true; }
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GENERICSENSOR_H
