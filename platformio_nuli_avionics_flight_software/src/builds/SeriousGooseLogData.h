#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIOUSGOOSELOGDATA_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIOUSGOOSELOGDATA_H

#include "core/BasicLoggerHelper.h"

// clang-format off
LOG_DATA_AUTO_DEFINE(HEADER_OUTPUT,
    struct SillyGooseLogData {
        uint32_t timestampMs;
        float pressurePa, barometerTemperatureK;
        float accelerationMSS_x, accelerationMSS_y, accelerationMSS_z, velocityRadS_x, velocityRadS_y, velocityRadS_z, imuTemperatureK;
        float magFieldTeslaRaw_x, magFieldTeslaRaw_y, magFieldTeslaRaw_z;
        float batteryVoltageV, altitudeM, velocityMS, accelerationMSS, unfilteredAltitudeM;
        int32_t flightState;
        uint8_t drogueState; bool drogueFired;
        uint8_t mainState; bool mainFired;
        uint8_t auxState; bool auxFired;
        float tiltMagnitudeDeg, angularVelRadS_x, angularVelRadS_y, angularVelRadS_z, quaternion_a, quaternion_b, quaternion_c, quaternion_d;
        float gpsLatitudeDeg, gpsLongitudeDeg, gpsAltitudeM;
        uint32_t gpsUnixTimeS;
        uint16_t gpsHdop, gpsVdop; // raw, scaled by 100 - matches the GPS module's native representation
        uint8_t gpsFixQuality, gpsSatellitesTracked;
    } remove_struct_padding;
)
// clang-format on

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIOUSGOOSELOGDATA_H
