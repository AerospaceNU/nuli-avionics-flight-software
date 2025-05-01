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
    float latitude;
    float longitude;
    float altitude;
    uint8_t fixQuality;
    uint8_t satsTracked;
    float hdop;

 

};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GPS_H
