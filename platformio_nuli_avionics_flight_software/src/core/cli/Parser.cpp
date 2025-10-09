#include "Parser.h"
#include <stdexcept>

// parses inputs into appropriate flags.
CLIReturnCode_e Parser::parse(int argc, char** argv) {
    if (m_numFlagGroups == 0) {
        return CLI_PARSER_NO_FLAG_GROUP_PROVIDED;
    }

    if (argc <= 1) {
        return CLI_PARSER_NO_FLAGS_PROVIDED;
    }

    int argvPos = 1;

    // retrieve flag group and set leader flag
    FlagGroup_s* flagGroup;
    const char* flagGroupName = argv[argvPos];    // The flag group's name should always be the first argument
    bool matchedLeader = false;
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        if (strcmp(flagGroupName, m_flagGroups[i].flagGroupName_s) == 0) {
            // flag group has been identified
            matchedLeader = true;
            flagGroup = &m_flagGroups[i];

            // set and parse the leader flag
            BaseFlag* leaderFlag = flagGroup->getLeader();

            // these branches don't represent SimpleFlag or ArgumentFlag
            if (argvPos + 1 >= argc || argv[argvPos + 1][0] == '-') {
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

    // set latest flag group
    m_latestFlagGroup = flagGroup->uid_s;

    // if any, go through rest of arguments
    for (; argvPos < argc; ++argvPos) {
        const char* currArg = argv[argvPos];
        bool matched = false;

        // find the matching flag
        for (uint8_t i = 0; i < flagGroup->numFlags_s; ++i) {
            if (strcmp(currArg, flagGroup->flags_s[i]->name()) == 0) {
                matched = true;

                CLIReturnCode_e parseResult;
                // these branches don't represent SimpleFlag or ArgumentFlag
                if (argvPos + 1 >= argc || argv[argvPos + 1][0] == '-') {
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

    // verify all required flags are set for this FlagGroup_s
    // **note**: conditional requirements are not checked for   //@TODO: Perhaps implement a system for this?
    return flagGroup->verifyFlags();
}

CLIReturnCode_e Parser::parse(char* input) {
    /* Diagram represent how the parser works without dynamic memory allocation.
     *   [--config 5 -C "hello world"]
     *           |- null
     *           |  |- null
     *           |  |   |- null
     *           |  |   |            |- null
     *           v  v   v            v
     *   --config\05\0-C\0hello world\0
     *   ^         ^   ^  ^
     *   |         |   |  |- pointer
     *   |         |   |- pointer
     *   |         |- pointer
     *   |-- pointer
     */

    int argc = 1;   // compatibility with command-line argc/argv
    char* argv[255] = {nullptr};

    char* p = input;

    while(*p) {
        switch(*p) {
            case '\t':
            case '\n':
            case ' ':
                *p = '\0';  // mark the end of a string
                p++;
                break;
            case '"':
                p++;    // skip the quote
                argv[argc++] = p;

                // loop until end of quote or input
                p = getString(p, '"');

                break;
            default:
                argv[argc++] = p;

                // loop until end of word
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

void Parser::printHelp() const {
    // loop through each FlagGroup_s
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        m_flagGroups[i].printHelp();
        printf("\n");
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
    // loop through each FlagGroup_s
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        // reset each flag group
        m_flagGroups[i].resetFlags();
    }

    // set back to unused
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

CLIReturnCode_e Parser::runFlags() {
    // retrieve the most recent flag group
    if (m_latestFlagGroup < 0) {
        return CLI_PARSER_MISSING_LATEST_FLAG_GROUP;
    }

    FlagGroup_s *flagGroup;
    CLIReturnCode_e returnCode = getFlagGroup(m_latestFlagGroup, &flagGroup);
    if (returnCode != CLI_SUCCESS) {
        return returnCode;
    }

    flagGroup->runFlags();
    return CLI_SUCCESS;
}

/* /////////////// */
/* / FlagGroup_s / */
/* /////////////// */

Parser::FlagGroup_s::FlagGroup_s(BaseFlag* flags[], const char* flagGroupName, uint8_t numFlags,
                                 int8_t uid)
        : flagGroupName_s(flagGroupName), numFlags_s(numFlags),
          uid_s(uid) {
    // check flag count
    if (numFlags > MAX_FLAGS) {
        return;
    }

    // copy flags into flags_s
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

void Parser::FlagGroup_s::printHelp() const {
    // loop through each set of flags within a FlagGroup_s
    for (uint8_t i = 0; i < numFlags_s; ++i) {
        printf("%s [%s]: %s\n", flags_s[i]->name(), flags_s[i]->isRequired() ? "Required" : "Optional", flags_s[i]->help());
    }
}

void Parser::FlagGroup_s::resetFlags() {
    for (uint8_t i = 0; i < numFlags_s; ++i) {
        flags_s[i]->reset();
    }
}

void Parser::FlagGroup_s::runFlags() {
    for (uint8_t i = 0; i < numFlags_s; ++i) {
        if (flags_s[i]->isSet()) flags_s[i]->run(uid_s);
    }
}

int Parser::FlagGroup_s::strcmp(const char* string1, const char* string2) { // NOLINT(*-convert-member-functions-to-static)
    while (*string1 && (*string1 == *string2)) {
        string1++;
        string2++;
    }

    return *(const unsigned char*)string1 - *(const unsigned char*)string2;
}
