#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRY_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRY_H

#include <Avionics.h>

struct TestStruct_s {
    int a;
    double b;
    uint8_t c;
};

enum ConfigurationID_e : int32_t {
    NONE = -1,
    // These must be in numerical order, with no gaps (1, 2, 3 NOT 1, 3, 4)
    CONFIGURATION_VERSION = 0,
    CONFIGURATION_CRC = 1,
    FLASH_START_LOC = 2,
    GROUND_ELEVATION = 3,
    GROUND_TEMPERATURE = 4,
    RADIO_FREQUENCY = 5,
    DODAD_THING = 6,
    // Leave this last
    LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE,
};

template<signed N>
struct GetConfigType_s;
template<> struct GetConfigType_s<NONE> {using type = void; };
template<> struct GetConfigType_s<CONFIGURATION_VERSION> {using type = uint32_t; };
template<> struct GetConfigType_s<CONFIGURATION_CRC> {using type = uint32_t; };
template<> struct GetConfigType_s<FLASH_START_LOC> {using type = uint32_t; };
template<> struct GetConfigType_s<GROUND_ELEVATION> {using type = double; };
template<> struct GetConfigType_s<GROUND_TEMPERATURE> {using type = double; };
template<> struct GetConfigType_s<RADIO_FREQUENCY> {using type = double; };
template<> struct GetConfigType_s<DODAD_THING> {using type = TestStruct_s; };
// Leave this last
template<> struct GetConfigType_s<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE> {using type = void; };

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////// DO NOT TOUCH ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////


struct ConfigSet_s {
    const ConfigurationID_e* data;
    uint16_t length;

    // Templated constructor to deduce length from array reference
    // This must ALLOW for implicit conversion, to keep syntax clean
    template <size_t N>
    ConfigSet_s(const ConfigurationID_e (&arr)[N]) : data(arr), length(N) {}
};

template<signed N> inline uint16_t getConfigLengthGenerator(ConfigurationID_e name) {
    if (name == N) return sizeof(typename GetConfigType_s<N>::type);
    return getConfigLengthGenerator<N - 1>(name);
}

template<> inline uint16_t getConfigLengthGenerator<-1>(ConfigurationID_e name) {
    return 0;
}

template<> inline uint16_t getConfigLengthGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE>(ConfigurationID_e name) {
    return getConfigLengthGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE - 1>(name);
}

inline uint16_t getConfigLength(ConfigurationID_e name) {
    return getConfigLengthGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE>(name);
}

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRY_H
