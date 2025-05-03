#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GPS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GPS_H

#include <Avionics.h>
#include "GenericSensor.h"

class GPS : public GenericSensor {
public:
    virtual ~GPS() = default;  // Ensure proper cleanup

    // Getter methods for GPS data
    float getLatitude() const { return latitude; }

    float getLongitude() const { return longitude; }

    float getAltitude() const { return altitude; }

    uint8_t getFixQuality() const { return fixQuality; }

    uint8_t getSatellitesTracked() const { return satsTracked; }

    float getHDOP() const { return hdop; }


protected:
    float latitude = 0;
    float longitude = 0;
    float altitude = 0;
    uint8_t fixQuality = 0;
    uint8_t satsTracked = 0;
    float hdop = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GPS_H
