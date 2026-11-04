#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GENERICSENSOR_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GENERICSENSOR_H

#include <Avionics.h>
#include "DebugStream.h"
#include "GenericAvionicsHardware.h"

/** @brief Base for all sensors. */
class GenericSensor : public GenericAvionicsHardware {
public:
    virtual ~GenericSensor() = default;
    /**
     * @brief Initialize the sensor
     * @details Enabling any peripherals, confirm sensor is talking, set configuration registers on the sensor
     */
    void setup(DebugStream *debugStream, WatchdogTimer *watchdog) override {}

    /**
     * @brief Read data from the sensor
     * @details Reads one reading from the sensor and converts it to useful units - may block the loop briefly (a few ms) waiting on the sensor.
     */
    void read() override {}


    virtual bool validReading() { return true; }
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GENERICSENSOR_H
