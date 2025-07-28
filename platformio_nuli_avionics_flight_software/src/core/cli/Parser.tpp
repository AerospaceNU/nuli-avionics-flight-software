#ifndef DESKTOP_PARSER_TPP
#define DESKTOP_PARSER_TPP

#include "Parser.h"

template<uint8_t n>
CLIReturnCode_e Parser::addFlagGroup(BaseFlag* (&flagGroup)[n]) {
    // bounds checks
    if (n == 0) {
        fprintf(stderr, "No flag group provided\n");
        return CLI_PARSER_ZERO_FLAG_GROUPS_ADDED;
    }

    if (m_numFlagGroups > MAX_FLAG_GROUPS) {
        fprintf(stderr, "Maximum flag groups exceeded\n");
        return CLI_PARSER_MAXIMUM_FLAG_GROUPS_EXCEEDED;
    }

    FlagGroup_s newFlagGroup(flagGroup, flagGroup[0]->name(), n, m_uid++);

    m_flagGroups[m_numFlagGroups++] = newFlagGroup;

    return CLI_SUCCESS;
}

template<uint8_t n>
CLIReturnCode_e Parser::addFlagGroup(BaseFlag* (&flagGroup)[n], int8_t uid) {
    // bounds checks
    if (n == 0) {
        fprintf(stderr, "No flag group provided\n");
        return CLI_PARSER_ZERO_FLAG_GROUPS_ADDED;
    }

    if (m_numFlagGroups > MAX_FLAG_GROUPS) {
        fprintf(stderr, "Maximum flag groups exceeded\n");
        return CLI_PARSER_MAXIMUM_FLAG_GROUPS_EXCEEDED;
    }

    FlagGroup_s newFlagGroup(flagGroup, flagGroup[0]->name(), n, uid);

    m_flagGroups[m_numFlagGroups++] = newFlagGroup;

    return CLI_SUCCESS;
}

template<typename T>
CLIReturnCode_e Parser::getValue(const char* flagGroupName, const char* flagName, T &value)  {
    // find flagGroup
    FlagGroup_s* flagGroups;
    if (getFlagGroup(flagGroupName, &flagGroups) < 0) {
        fprintf(stderr, "FlagGroup not found for: %s\n", flagGroupName);
        return CLI_PARSER_FLAG_GROUP_NOT_FOUND;
    }

    BaseFlag* flag;
    if (flagGroups->getFlag(flagName, &flag) < 0) {
        fprintf(stderr, "Unable to find flag: %s\n", flagName);
        return CLI_PARSER_FLAG_NOT_FOUND;
    }

    value = (flag->getValue<T>());
    return CLI_SUCCESS;
}

#endif // DESKTOP_PARSER_TPP