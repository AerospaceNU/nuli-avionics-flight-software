#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRYWRAP_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRYWRAP_H

#include <Avionics.h>

typedef int16_t ConfigurationID_t;

// This creates a unique ID for each config name, creates and enum alias,
// and a struct that allows type to be determined from ID at compile time
template <signed N>
struct GetConfigurationType_s;
enum { CONFIGURATION_COUNT_BASE = __COUNTER__ };
#define DEFINE_CONFIGURATION_VARIABLE(CONFIGURATION_ENUM, dataType, DefaultValue) enum class Dasf328ThisOnlyExistsToMakeConfigurationRegistryLookPrettyAndCanBeRemovedkasdfhkdasfa##CONFIGURATION_ENUM { CONFIGURATION_ENUM }; enum : int16_t { CONFIGURATION_ENUM##_c = (__COUNTER__ - CONFIGURATION_COUNT_BASE - 2) }; template<> struct GetConfigurationType_s<CONFIGURATION_ENUM##_c> {using type = dataType; static constexpr const char * name = #CONFIGURATION_ENUM; static constexpr const char * command = "--" #CONFIGURATION_ENUM; static constexpr const dataType defaultValue = DefaultValue; };

// Register all the configurables
DEFINE_CONFIGURATION_VARIABLE(NONE, int, 0)                                               // Base case
#include "../ConfigurationRegistry.h"                                                   // All user defined variables
DEFINE_CONFIGURATION_VARIABLE(LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE, int, 0)      // End case

struct ConfigurationIDSet_s {
    const ConfigurationID_t* data;
    uint16_t length;

    // Templated constructor to deduce length from array reference
    // This must ALLOW for implicit conversion, to keep syntax clean
    template <size_t N>
    ConfigurationIDSet_s(const ConfigurationID_t (&arr)[N]) : data(arr), length(N) {}
};

template <signed N>
inline uint16_t getConfigurationLengthGenerator(const ConfigurationID_t name) {
    if (name == N) return sizeof(typename GetConfigurationType_s<N>::type);
    return getConfigurationLengthGenerator<N - 1>(name);
}

template <>
inline uint16_t getConfigurationLengthGenerator<-1>(ConfigurationID_t name) {
    return 0;
}

template <>
inline uint16_t getConfigurationLengthGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c>(const ConfigurationID_t name) {
    return getConfigurationLengthGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c - 1>(name);
}

inline uint16_t getConfigurationLength(const ConfigurationID_t name) {
    return getConfigurationLengthGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c>(name);
}

// Recursive generator to get the name of a given configuration ID
template <signed N>
inline const char* getConfigurationNameGenerator(const ConfigurationID_t name) {
    if (name == N) return GetConfigurationType_s<N>::name;
    return getConfigurationNameGenerator<N - 1>(name);
}

// Base case specialization: stop recursion at -1
template <>
inline const char* getConfigurationNameGenerator<-1>(const ConfigurationID_t) {
    return "NONE";
}

// Skip over the last placeholder entry
template <>
inline const char* getConfigurationNameGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c>(const ConfigurationID_t name) {
    return getConfigurationNameGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c - 1>(name);
}

// Public function
inline const char* getConfigurationName(const ConfigurationID_t name) {
    return getConfigurationNameGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c>(name);
}

inline bool strEquals(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        ++a;
        ++b;
    }
    return *a == *b;
}

// Recursive generator to get ID from exact string
template <signed N>
inline ConfigurationID_t getConfigurationIDGenerator(const char* str) {
    if (strEquals(str, GetConfigurationType_s<N>::name)) {
        return static_cast<ConfigurationID_t>(N);
    }
    return getConfigurationIDGenerator<N - 1>(str);
}

// Base case
template <>
inline ConfigurationID_t getConfigurationIDGenerator<-1>(const char*) {
    return NONE_c;
}

// Skip over placeholder
template <>
inline ConfigurationID_t getConfigurationIDGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c>(const char* str) {
    return getConfigurationIDGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c - 1>(str);
}

// Public function
inline ConfigurationID_t getConfigurationID(const char* str) {
    return getConfigurationIDGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c>(str);
}

// Recursive generator to get the name of a given configuration ID
template <signed N>
inline void getConfigurationDefaultGenerator(const ConfigurationID_t name, void* dst) {
    if (name == N) {
        using config_type_t = typename GetConfigurationType_s<N>::type;
        *((config_type_t *) dst) = GetConfigurationType_s<N>::defaultValue;
        return;
    }
    return getConfigurationDefaultGenerator<N - 1>(name, dst);
}

// Base case specialization: stop recursion at -1
template <>
inline void getConfigurationDefaultGenerator<-1>(const ConfigurationID_t, void* dst) {

}

// Skip over the last placeholder entry
template <>
inline void getConfigurationDefaultGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c>(const ConfigurationID_t name, void* dst) {
    return getConfigurationDefaultGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c - 1>(name, dst);
}

// Public function
inline void getConfigurationDefault(const ConfigurationID_t name, void* dst) {
    getConfigurationDefaultGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c>(name, dst);
}


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRYWRAP_H
