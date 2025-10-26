#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRYWRAP_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRYWRAP_H

#include "Avionics.h"
#include "ConstantsUnits.h"
#include <cstring>

typedef int16_t ConfigurationID_t;

template <unsigned N>
struct ConfigurationString {
    char str[N]{};

    enum { MAX_LENGTH = N - 1 };

    ConfigurationString() {
        str[0] = '\0';
    }

    ConfigurationString(const char* s) {
        std::strncpy(str, s, N - 1);
        str[N - 1] = '\0'; // ensure null termination
    }
};

// This creates a unique ID for each config name, creates and enum alias,
// and a struct that allows type to be determined from ID at compile time
template <signed N>
struct GetConfigurationType_s;

enum { CONFIGURATION_COUNT_BASE = __COUNTER__ };

// #define DEFINE_CONFIGURATION_VARIABLE(CONFIGURATION_ENUM, dataType, DefaultValue) enum class Dasf328ThisOnlyExistsToMakeConfigurationRegistryLookPrettyAndCanBeRemovedkasdfhkdasfa##CONFIGURATION_ENUM { CONFIGURATION_ENUM }; enum : int16_t { CONFIGURATION_ENUM##_c = (__COUNTER__ - CONFIGURATION_COUNT_BASE - 2) }; template<> struct GetConfigurationType_s<CONFIGURATION_ENUM##_c> {using type = dataType; static constexpr const char * name = #CONFIGURATION_ENUM; static constexpr const char * command = "--" #CONFIGURATION_ENUM; static constexpr const dataType defaultValue = DefaultValue; };
#define DEFINE_CONFIGURATION_VARIABLE(CONFIGURATION_ENUM, dataType, DefaultValue, condition) \
    enum class Dasf328ThisOnlyExistsToMakeConfigurationRegistryLookPrettyAndCanBeRemovedkasdfhkdasfa##CONFIGURATION_ENUM { CONFIGURATION_ENUM }; \
    enum : int16_t { CONFIGURATION_ENUM##_c = (__COUNTER__ - CONFIGURATION_COUNT_BASE - 2) }; \
    template<> struct GetConfigurationType_s<CONFIGURATION_ENUM##_c> { \
        using type = dataType; static constexpr const char * name = #CONFIGURATION_ENUM; \
        static constexpr const char * command = "--" #CONFIGURATION_ENUM; \
        static dataType defaultValue() { return DefaultValue; }; \
        static bool isValid(const dataType* valuePtr) { const dataType& value = *valuePtr; (void)value; return condition; } \
    };

// Register all the configurables
DEFINE_CONFIGURATION_VARIABLE(NONE, int, 0, true) // Base case
DEFINE_CONFIGURATION_VARIABLE(CONFIGURATION_CRC, uint32_t, 0, true)
DEFINE_CONFIGURATION_VARIABLE(CONFIGURATION_ALL_ID_CRC, uint32_t, 0, true)
DEFINE_CONFIGURATION_VARIABLE(CONFIGURATION_VERSION, uint32_t, 1, true)
#include "../../ConfigurationRegistry.h"
DEFINE_CONFIGURATION_VARIABLE(LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE, int, 0, true) // End case

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

template <int N>
inline void getConfigurationDefaultGenerator(ConfigurationID_t id, void* dst) {
    if (id == N) {
        typedef typename GetConfigurationType_s<N>::type config_type_t;
        *((config_type_t*)dst) = GetConfigurationType_s<N>::defaultValue();
        return;
    }
    getConfigurationDefaultGenerator<N - 1>(id, dst);
}

// Base case specialization: stop recursion at -1
template <>
inline void getConfigurationDefaultGenerator<-1>(ConfigurationID_t, void*) {
    // nothing
}

// Special case to skip over last "sentinel" ID
template <>
inline void getConfigurationDefaultGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c>(ConfigurationID_t id, void* dst) {
    getConfigurationDefaultGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c - 1>(id, dst);
}

// Public entry point — this is what you call at runtime
inline void getConfigurationDefault(ConfigurationID_t id, void* dst) {
    getConfigurationDefaultGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c>(id, dst);
}


// Recursive generator to check validity for a given configuration ID.
template <int N>
inline bool getConfigurationValidGenerator(ConfigurationID_t id, const void* src) {
    if (id == N) {
        typedef typename GetConfigurationType_s<N>::type config_type_t;
        return GetConfigurationType_s<N>::isValid(static_cast<const config_type_t*>(src));
    }
    return getConfigurationValidGenerator<N - 1>(id, src);
}

// Base case specialization: stop recursion at -1
template <>
inline bool getConfigurationValidGenerator<-1>(ConfigurationID_t, const void*) {
    return false; // default if ID not found
}

// Skip the sentinel entry
template <>
inline bool getConfigurationValidGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c>(
    ConfigurationID_t id, const void* src)
{
    return getConfigurationValidGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c - 1>(id, src);
}

// Public function
inline bool getConfigurationValid(ConfigurationID_t id, const void* src) {
    return getConfigurationValidGenerator<LEAVE_THIS_ENTRY_LAST_WITH_THE_HIGHEST_VALUE_c>(id, src);
}


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRYWRAP_H
