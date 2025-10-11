#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONSTANTSUNITS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONSTANTSUNITS_H

class Constants {
public:
    static constexpr double PI_VAL = 3.1415926535897932384626433832795;
    static constexpr double G_EARTH_MSS = 9.80665;
    static constexpr double ATMOSPHERIC_PRESSURE_PA = 101325;
    static constexpr double LAPSE_RATE_K_M = -0.0065;
    static constexpr double GAS_CONSTANT_J_KG_K = 287.0474909;
    static constexpr double STANDARD_TEMPERATURE_K = 273.15;
};

class Units {
public:
    static constexpr double MICRO_TO_BASE = 1.0 / 1000000.0;
    static constexpr double MILLI_TO_BASE = 1.0 / 1000.0;

    static constexpr double MBAR_TO_PA = 100;
    static constexpr double DEGS_TO_RAD = Constants::PI_VAL / 180.0;
    static constexpr double C_TO_K = 273.15;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONSTANTSUNITS_H
