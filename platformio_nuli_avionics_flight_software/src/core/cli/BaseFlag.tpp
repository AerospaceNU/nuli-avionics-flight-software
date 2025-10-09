#ifndef DESKTOP_BASEFLAG_TPP
#define DESKTOP_BASEFLAG_TPP

#include "BaseFlag.h"
// #include <sstream>

// general
template<typename T>
CLIReturnCode_e BaseFlag::parseArgument(const char* value, T &result) {
    // std::istringstream iss(value);
    // iss >> result;
    // if (iss.fail()) {
    //     return CLI_PARSE_FAILED_TO_PARSE_ARGUMENT;
    // }
    //
    // // Check for leftover characters (ensures complete parsing)
    // char leftover;
    // if (iss >> leftover) {
    //     return CLI_PARSE_FAIL_EXTRA_ARGUMENTS;
    // }

    return CLI_SUCCESS;
}

// specialization for const char*
template<>
inline CLIReturnCode_e BaseFlag::parseArgument<const char*>(const char* value, const char*&result) {
    result = value;
    return CLI_SUCCESS;
}

template<typename T>
T BaseFlag::getValue() {
    T value;
    getValueRaw(&value);  // Call the virtual function
    return value;
}

#endif // DESKTOP_BASEFLAG_TPP
