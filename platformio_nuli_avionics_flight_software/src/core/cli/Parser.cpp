#include "Parser.h"
#include "core/generic_hardware/DebugStream.h"
#include <algorithm>

CLIReturnCode_e Parser::parse(int argc, char** argv) {
    if (m_numFlagGroups == 0) {
        return CLI_PARSER_NO_FLAG_GROUP_PROVIDED;
    }

    if (argc <= 1) {
        return CLI_PARSER_NO_FLAGS_PROVIDED;
    }

    int argvPos = 1;

    FlagGroup_s* flagGroup;
    const char* flagGroupName = argv[argvPos];
    bool matchedLeader = false;
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        if (strcmp(flagGroupName, m_flagGroups[i].flagGroupName_s) == 0) {
            matchedLeader = true;
            flagGroup = &m_flagGroups[i];

            BaseFlag* leaderFlag = flagGroup->getLeader();

            if (argvPos + 1 >= argc || this->isKnownFlag(argv[(argvPos) + 1], *flagGroup)) {
                leaderFlag->parse(nullptr);
            } else {
                leaderFlag->parse(argv[argvPos + 1]);
                argvPos += 2;
            }
        }

        if (matchedLeader) {
            break;
        }
    }

    if (!matchedLeader) {
        return CLI_PARSER_NO_LEADER_FLAG;
    }

    m_latestFlagGroup = flagGroup->uid_s;

    for (; argvPos < argc; ++argvPos) {
        const char* currArg = argv[argvPos];
        bool matched = false;

        for (uint8_t i = 0; i < flagGroup->numFlags_s; ++i) {
            if (strcmp(currArg, flagGroup->flags_s[i]->name()) == 0) {
                matched = true;

                CLIReturnCode_e parseResult;
                if (argvPos + 1 >= argc || this->isKnownFlag(argv[argvPos + 1], *flagGroup)) {
                    parseResult = flagGroup->flags_s[i]->parse(nullptr);
                } else {
                    parseResult = flagGroup->flags_s[i]->parse(argv[argvPos + 1]);
                    argvPos++;
                }

                if (parseResult < 0) {
                    return parseResult;
                }
            }

            if (matched) {
                break;
            }
        }

        if (!matched) {
            return CLI_PARSER_UNKNOWN_FLAG;
        }
    }

    return flagGroup->verifyFlags();
}

CLIReturnCode_e Parser::parse(char* input) {
    // Tokenizes in place (no allocation): separators become '\0' and argv points into input.

    int argc = 1;   // argv[0] unused, matching command-line argc/argv
    char* argv[255] = {nullptr};

    char* p = input;

    while(*p) {
        switch(*p) {
            case '\t':
            case '\r':
            case '\n':
            case ' ':
                *p = '\0';
                p++;
                break;
            case '"':
                p++;
                argv[argc++] = p;

                p = getString(p, '"');

                break;
            default:
                argv[argc++] = p;

                p = getString(p, ' ');

                break;
        }
    }

    argv[argc] = nullptr;
    return this->parse(argc, argv);
}

char* Parser::getString(char* p, char target) const { // NOLINT(*-convert-member-functions-to-static)
    while(*p && *p != target && *p != '\n' && *p != '\r') {
        p++;
    }

    if (*p) {
        *p = '\0';
        p++;
    }

    return p;
}

int Parser::strcmp(const char* string1, const char* string2) {  // NOLINT(*-convert-member-functions-to-static)
    while (*string1 && (*string1 == *string2)) {
        string1++;
        string2++;
    }

    return *(const unsigned char*)string1 - *(const unsigned char*)string2;
}

void Parser::printHelp(DebugStream* debugStream) const {
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        m_flagGroups[i].printHelp(debugStream);
        debugStream->data("");
    }
}

CLIReturnCode_e Parser::FlagGroup_s::verifyFlags() {
    for (uint8_t i = 0; i < this->numFlags_s; ++i) {
        if (!this->flags_s[i]->verify()) {
            return CLI_PARSER_MISSING_REQUIRED_ARGS;
        }
    }

    return CLI_SUCCESS;
}

void Parser::resetFlags() {
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        m_flagGroups[i].resetFlags();
    }

    m_latestFlagGroup = -1;
}

CLIReturnCode_e Parser::getFlagGroup(const char* flagGroupName, Parser::FlagGroup_s** flagGroup) {
    for (int i = 0; i < m_numFlagGroups; ++i) {
        if (strcmp(flagGroupName, m_flagGroups[i].flagGroupName_s) == 0) {
            *flagGroup = &m_flagGroups[i];
            return CLI_SUCCESS;
        }
    }

    return CLI_PARSER_UNKNOWN_FLAG_GROUP;
}

CLIReturnCode_e Parser::getFlagGroup(int8_t uid, FlagGroup_s** flagGroup) {
    for (int i = 0; i < m_numFlagGroups; ++i) {
        if (m_flagGroups[i].uid_s == uid) {
            *flagGroup = &m_flagGroups[i];
            return CLI_SUCCESS;
        }
    }

    return CLI_PARSER_UNKNOWN_FLAG_GROUP;
}

CLIReturnCode_e Parser::runFlags(DebugStream* debugStream) {
    if (m_latestFlagGroup < 0) {
        return CLI_PARSER_MISSING_LATEST_FLAG_GROUP;
    }

    FlagGroup_s *flagGroup;
    CLIReturnCode_e returnCode = getFlagGroup(m_latestFlagGroup, &flagGroup);
    if (returnCode != CLI_SUCCESS) {
        return returnCode;
    }

    flagGroup->runFlags(debugStream);
    return CLI_SUCCESS;
}

/* /////////////// */
/* / FlagGroup_s / */
/* /////////////// */

Parser::FlagGroup_s::FlagGroup_s(BaseFlag* flags[], const char* flagGroupName, uint8_t numFlags,
                                 int8_t uid)
        : flagGroupName_s(flagGroupName), numFlags_s(numFlags),
          uid_s(uid) {
    if (numFlags > MAX_FLAGS) {
        return;
    }

    std::copy(flags, flags + numFlags, flags_s);
}

BaseFlag* Parser::FlagGroup_s::getLeader() {
    return flags_s[0];
}

CLIReturnCode_e Parser::FlagGroup_s::getFlag(const char* flagName, BaseFlag** flag) {
    for (int i = 0; i < numFlags_s; ++i) {
        if (strcmp(flagName, flags_s[i]->name()) == 0) {
            *flag = flags_s[i];
            return CLI_SUCCESS;
        }
    }

    return CLI_PARSER_UNKNOWN_FLAG;
}

void Parser::FlagGroup_s::printHelp(DebugStream* debugStream) const {
    for (uint8_t i = 0; i < numFlags_s; ++i) {
        const char* indent = (i == 0) ? "" : "    ";
        const char* required = flags_s[i]->isRequired() ? "Required" : "Optional";
        const char* helpText = flags_s[i]->help();
        if (helpText && helpText[0] != '\0') {
            debugStream->data("%s%s [%s]: %s", indent, flags_s[i]->name(), required, helpText);
        } else {
            debugStream->data("%s%s [%s]", indent, flags_s[i]->name(), required);
        }
    }
}

void Parser::FlagGroup_s::resetFlags() {
    for (uint8_t i = 0; i < numFlags_s; ++i) {
        flags_s[i]->reset();
    }
}

void Parser::FlagGroup_s::runFlags(DebugStream* debugStream) {
    for (uint8_t i = 0; i < numFlags_s; ++i) {
        if (flags_s[i]->isSet() && (!flags_s[i]->isHighBandwidthOnly() || debugStream->isHighBandwidth())) {
            flags_s[i]->run(debugStream);
        }
    }
}

int Parser::FlagGroup_s::strcmp(const char* string1, const char* string2) { // NOLINT(*-convert-member-functions-to-static)
    while (*string1 && (*string1 == *string2)) {
        string1++;
        string2++;
    }

    return *(const unsigned char*)string1 - *(const unsigned char*)string2;
}
    bool Parser::isKnownFlag(char* arg, Parser::FlagGroup_s &flagGroupS) {
        for (int i = 0; i < flagGroupS.numFlags_s; i++) {
            if (this->strcmp(arg, flagGroupS.flags_s[i]->name()) == 0) {
                return true;
            }
        }
        return false;
    }
