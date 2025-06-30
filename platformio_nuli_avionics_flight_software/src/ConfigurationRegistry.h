#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRY_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRY_H

#include <Avionics.h>

//////// Place any structs used by the config below


//////// Registry below

enum ConfigurationID_e : int16_t {
    // These must be in numerical order, with no gaps (1, 2, 3 NOT 1, 3, 4)
    NONE = -1,
    CONFIGURATION_VERSION = 0,
    CONFIGURATION_VERSION_HASH = 1,
    CONFIGURATION_CRC = 2,
    FLASH_START_LOC,
    GROUND_ELEVATION,
    GROUND_TEMPERATURE,
    RADIO_FREQUENCY,
    MAIN_ELEVATION,
    DROGUE_DELAY,
    // Leave this last
    LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE,
};

template<signed N>
struct GetConfigType_s;
template<> struct GetConfigType_s<NONE> {using type = void; };
template<> struct GetConfigType_s<CONFIGURATION_VERSION> {using type = uint32_t; };
template<> struct GetConfigType_s<CONFIGURATION_VERSION_HASH> {using type = uint32_t; };
template<> struct GetConfigType_s<CONFIGURATION_CRC> {using type = uint32_t; };
template<> struct GetConfigType_s<FLASH_START_LOC> {using type = uint32_t; };
template<> struct GetConfigType_s<GROUND_ELEVATION> {using type = float; };
template<> struct GetConfigType_s<GROUND_TEMPERATURE> {using type = float; };
template<> struct GetConfigType_s<RADIO_FREQUENCY> {using type = float; };
template<> struct GetConfigType_s<MAIN_ELEVATION> {using type = float; };
template<> struct GetConfigType_s<DROGUE_DELAY> {using type = float; };
// Leave this last
template<> struct GetConfigType_s<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE> {using type = void; };

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////// DO NOT TOUCH ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

struct ConfigurationIDSet_s {
    const ConfigurationID_e* data;
    uint16_t length;

    // Templated constructor to deduce length from array reference
    // This must ALLOW for implicit conversion, to keep syntax clean
    template <size_t N>
    ConfigurationIDSet_s(const ConfigurationID_e (&arr)[N]) : data(arr), length(N) {}
};

template<signed N> inline uint16_t getConfigurationLengthGenerator(ConfigurationID_e name) {
    if (name == N) return sizeof(typename GetConfigType_s<N>::type);
    return getConfigurationLengthGenerator<N - 1>(name);
}

template<> inline uint16_t getConfigurationLengthGenerator<-1>(ConfigurationID_e name) {
    return 0;
}

template<> inline uint16_t getConfigurationLengthGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE>(ConfigurationID_e name) {
    return getConfigurationLengthGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE - 1>(name);
}

inline uint16_t getConfigurationLength(ConfigurationID_e name) {
    return getConfigurationLengthGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE>(name);
}

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRY_H
