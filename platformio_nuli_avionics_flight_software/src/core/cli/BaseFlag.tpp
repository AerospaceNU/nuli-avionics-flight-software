#ifndef DESKTOP_BASEFLAG_TPP
#define DESKTOP_BASEFLAG_TPP

// int8_t
template<>
inline CLIReturnCode_e BaseFlag::parseArgument(const char* value, int8_t& result) {
    char* end;
    long val = std::strtol(value, &end, 10);
    if (*end != '\0' || val < INT8_MIN || val > INT8_MAX) {
        return CLI_PARSE_FAILED_TO_PARSE_ARGUMENT;
    }
    result = static_cast<int8_t>(val);
    return CLI_SUCCESS;
}

// uint8_t
template<>
inline CLIReturnCode_e BaseFlag::parseArgument(const char* value, uint8_t& result) {
    char* end;
    unsigned long val = std::strtoul(value, &end, 10);
    if (*end != '\0' || val > UINT8_MAX) {
        return CLI_PARSE_FAILED_TO_PARSE_ARGUMENT;
    }
    result = static_cast<uint8_t>(val);
    return CLI_SUCCESS;
}

// int16_t
template<>
inline CLIReturnCode_e BaseFlag::parseArgument(const char* value, int16_t& result) {
    char* end;
    long val = std::strtol(value, &end, 10);
    if (*end != '\0' || val < INT16_MIN || val > INT16_MAX) {
        return CLI_PARSE_FAILED_TO_PARSE_ARGUMENT;
    }
    result = static_cast<int16_t>(val);
    return CLI_SUCCESS;
}

// uint16_t
template<>
inline CLIReturnCode_e BaseFlag::parseArgument(const char* value, uint16_t& result) {
    char* end;
    unsigned long val = std::strtoul(value, &end, 10);
    if (*end != '\0' || val > UINT16_MAX) {
        return CLI_PARSE_FAILED_TO_PARSE_ARGUMENT;
    }
    result = static_cast<uint16_t>(val);
    return CLI_SUCCESS;
}

// int32_t
template<>
inline CLIReturnCode_e BaseFlag::parseArgument(const char* value, int32_t& result) {
    char* end;
    long val = std::strtol(value, &end, 10);
    if (*end != '\0' || val < INT32_MIN || val > INT32_MAX) {
        return CLI_PARSE_FAILED_TO_PARSE_ARGUMENT;
    }
    result = static_cast<int32_t>(val);
    return CLI_SUCCESS;
}

// uint32_t
template<>
inline CLIReturnCode_e BaseFlag::parseArgument(const char* value, uint32_t& result) {
    char* end;
    unsigned long val = std::strtoul(value, &end, 10);
    if (*end != '\0' || val > UINT32_MAX) {
        return CLI_PARSE_FAILED_TO_PARSE_ARGUMENT;
    }
    result = static_cast<uint32_t>(val);
    return CLI_SUCCESS;
}

// float
template<>
inline CLIReturnCode_e BaseFlag::parseArgument(const char* value, float& result) {
    char* end;
    float val = std::strtof(value, &end);
    if (*end != '\0') {
        return CLI_PARSE_FAILED_TO_PARSE_ARGUMENT;
    }
    result = val;
    return CLI_SUCCESS;
}

// double
template<>
inline CLIReturnCode_e BaseFlag::parseArgument(const char* value, double& result) {
    char* end;
    double val = std::strtod(value, &end);
    if (*end != '\0') {
        return CLI_PARSE_FAILED_TO_PARSE_ARGUMENT;
    }
    result = val;
    return CLI_SUCCESS;
}

// specialization for const char*
template<>
inline CLIReturnCode_e BaseFlag::parseArgument<const char*>(const char* value, const char*&result) {
    result = value;
    return CLI_SUCCESS;
}

template<typename T>
CLIReturnCode_e BaseFlag::parseArgument(const char *value, T &result) {
    return CLI_PARSE_ARGUMENT_UNHANDLED_TYPE;
}

template<typename T>
T BaseFlag::getValue() {
    T value;
    getValueRaw(&value);  // Call the virtual function
    return value;
}

#endif // DESKTOP_BASEFLAG_TPP
