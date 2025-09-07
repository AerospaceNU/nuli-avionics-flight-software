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
    STATE,
    FLASH_START_LOC,
    GROUND_ELEVATION,
    GROUND_TEMPERATURE,
    RADIO_FREQUENCY,
    MAIN_ELEVATION,
    DROGUE_DELAY,
    // Leave this last
    LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE,
};

// This struct allows us to associate a give enum/config ID to a c++ primitive type
template<signed N>
struct GetConfigurationType_s;
#define ASSOCIATE_CONFIGURATION_TYPE(CONFIGURATION_ENUM, dataType) template<> struct GetConfigurationType_s<CONFIGURATION_ENUM> {using type = dataType; };
//// Register each configurable here
ASSOCIATE_CONFIGURATION_TYPE(NONE, void)
ASSOCIATE_CONFIGURATION_TYPE(CONFIGURATION_VERSION, uint32_t)
ASSOCIATE_CONFIGURATION_TYPE(CONFIGURATION_VERSION_HASH, uint32_t)
ASSOCIATE_CONFIGURATION_TYPE(CONFIGURATION_CRC, uint32_t)
ASSOCIATE_CONFIGURATION_TYPE(STATE, int32_t)
ASSOCIATE_CONFIGURATION_TYPE(FLASH_START_LOC, int32_t)
ASSOCIATE_CONFIGURATION_TYPE(GROUND_ELEVATION, float)
ASSOCIATE_CONFIGURATION_TYPE(GROUND_TEMPERATURE, float)
ASSOCIATE_CONFIGURATION_TYPE(RADIO_FREQUENCY, float)
ASSOCIATE_CONFIGURATION_TYPE(MAIN_ELEVATION, float)
ASSOCIATE_CONFIGURATION_TYPE(DROGUE_DELAY, uint32_t)
// Leave this last
ASSOCIATE_CONFIGURATION_TYPE(LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE, void);

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

template<signed N> inline uint16_t getConfigurationLengthGenerator(const ConfigurationID_e name) {
    if (name == N) return sizeof(typename GetConfigurationType_s<N>::type);
    return getConfigurationLengthGenerator<N - 1>(name);
}

template<> inline uint16_t getConfigurationLengthGenerator<-1>(ConfigurationID_e name) {
    return 0;
}

template<> inline uint16_t getConfigurationLengthGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE>(const ConfigurationID_e name) {
    return getConfigurationLengthGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE - 1>(name);
}

inline uint16_t getConfigurationLength(const ConfigurationID_e name) {
    return getConfigurationLengthGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE>(name);
}

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRY_H
