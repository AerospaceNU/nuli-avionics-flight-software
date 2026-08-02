#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GPS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GPS_H

#include <Avionics.h>
#include "GenericSensor.h"

class GPS : public GenericSensor {
public:
    ~GPS() override = default; // Ensure proper cleanup

    // Getter methods for GPS data
    Coordinates_s getCoordinates() const { return m_coordinates; }

    uint8_t getFixQuality() const { return m_fixQuality; }

    uint8_t getSatellitesTracked() const { return m_satellitesTracked; }

    /// Horizontal dilution of precision, scaled by 100 (matches the GPS module's native representation)
    uint16_t getHDOP() const { return m_hdop; }

    /// Vertical dilution of precision, scaled by 100 (matches the GPS module's native representation)
    uint16_t getVDOP() const { return m_vdop; }

    uint32_t getUnixTimeS() const { return m_unixTimeS; }

protected:
    Coordinates_s m_coordinates = {};
    uint8_t m_fixQuality = 0;
    uint8_t m_satellitesTracked = 0;
    uint16_t m_hdop = 0;
    uint16_t m_vdop = 0;
    uint32_t m_unixTimeS = 0; ///< GPS-reported time as a unix epoch timestamp (seconds since Jan 1st 1970)
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GPS_H
