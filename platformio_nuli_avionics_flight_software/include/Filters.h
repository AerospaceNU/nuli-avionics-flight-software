#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FILTERS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FILTERS_H

#include <Avionics.h>
#include <HardwareAbstraction.h>

struct FilteredSensorData {

};

class Filters {
public:
    void setup(Configuration* configuration, Logger* logger) {
        m_configuration = configuration;
        m_logger = logger;
    }

    FilteredSensorData runFilterOnce() {
        return {};
    }

private:
    Configuration* m_configuration;
    Logger* m_logger;
};


/**
 * Kalman:
 * - Perdict(accel)
 * - Calc gains
 * - Correct
 * -
 */

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_FILTERS_H
