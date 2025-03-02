#ifndef DESKTOP_BASEFLAG_TPP
#define DESKTOP_BASEFLAG_TPP

#include "BaseFlag.h"
#include <sstream>

// general
template<typename T>
int8_t BaseFlag::parseArgument(const char* value, T &result) {
    std::istringstream iss(value);
    iss >> result;
    if (iss.fail()) {
        fprintf(m_errorStream, "Failed to parse argument: %s\n", value);
        return -1;
    }

    // Check for leftover characters (ensures complete parsing)
    char leftover;
    if (iss >> leftover) {
        fprintf(m_errorStream, "Failed to parse argument (extra characters found): %s\n", value);
        return -1;
    }

    return 0;
}

// specialization for const char*
template<>
inline int8_t BaseFlag::parseArgument<const char*>(const char* value, const char*&result) {
    result = value;
    return 0;
}

template<typename T>
T BaseFlag::getValue() {
    T value;
    getValueRaw(&value);  // Call the virtual function
    return value;
}

#endif // DESKTOP_BASEFLAG_TPP
