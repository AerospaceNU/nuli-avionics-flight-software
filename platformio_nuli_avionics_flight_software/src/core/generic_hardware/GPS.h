#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GPS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GPS_H

#include <Avionics.h>
#include "GenericSensor.h"

class GPS : public GenericSensor {
public:
    ~GPS() override = default;

    Coordinates_s getCoordinates() const { return m_coordinates; }

    uint8_t getFixQuality() const { return m_fixQuality; }

    uint8_t getSatellitesTracked() const { return m_satellitesTracked; }

    /// HDOP x100 (the module's native representation)
    uint16_t getHDOP() const { return m_hdop; }

    /// VDOP x100 (the module's native representation)
    uint16_t getVDOP() const { return m_vdop; }

    uint32_t getUnixTimeS() const { return m_unixTimeS; }

protected:
    Coordinates_s m_coordinates = {};
    uint8_t m_fixQuality = 0;
    uint8_t m_satellitesTracked = 0;
    uint16_t m_hdop = 0;
    uint16_t m_vdop = 0;
    uint32_t m_unixTimeS = 0;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GPS_H
