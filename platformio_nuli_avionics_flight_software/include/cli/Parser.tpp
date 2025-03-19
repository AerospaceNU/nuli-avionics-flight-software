#ifndef DESKTOP_PARSER_TPP
#define DESKTOP_PARSER_TPP

#include "Parser.h"

template<uint8_t n>
int8_t Parser::addFlagGroup(BaseFlag* (&flagGroup)[n]) {
    // bounds checks
    if (n == 0) {
        fprintf(m_errorStream, "No flag group provided\n");
        return -1;
    }

    if (m_numFlagGroups > MAX_FLAG_GROUPS) {
        fprintf(m_errorStream, "Maximum flag groups exceeded\n");
        return -1;
    }

    FlagGroup_s newFlagGroup(flagGroup, flagGroup[0]->name(), n,
                             m_inputStream, m_outputStream, m_errorStream,
                             m_uid++);

    m_flagGroups[m_numFlagGroups++] = newFlagGroup;

    return 0;
}

template<uint8_t n>
int8_t Parser::addFlagGroup(BaseFlag* (&flagGroup)[n], int8_t uid) {
    // bounds checks
    if (n == 0) {
        fprintf(m_errorStream, "No flag group provided\n");
        return -1;
    }

    if (m_numFlagGroups > MAX_FLAG_GROUPS) {
        fprintf(m_errorStream, "Maximum flag groups exceeded\n");
        return -1;
    }

    FlagGroup_s newFlagGroup(flagGroup, flagGroup[0]->name(), n,
                             m_inputStream, m_outputStream, m_errorStream,
                             uid);

    m_flagGroups[m_numFlagGroups++] = newFlagGroup;

    return 0;
}

template<typename T>
int8_t Parser::getValue(const char* flagGroupName, const char* flagName, T &value)  {
    // find flagGroup
    FlagGroup_s* flagGroups;
    if (getFlagGroup(flagGroupName, &flagGroups) < 0) {
        fprintf(stderr, "FlagGroup not found for: %s\n", flagGroupName);
        return -1;
    }

    BaseFlag* flag;
    if (flagGroups->getFlag(flagName, &flag) < 0) {
        fprintf(stderr, "Unable to find flag: %s\n", flagName);
        return -1;
    }

    value = (flag->getValue<T>());
    return 0;
}

#endif // DESKTOP_PARSER_TPP