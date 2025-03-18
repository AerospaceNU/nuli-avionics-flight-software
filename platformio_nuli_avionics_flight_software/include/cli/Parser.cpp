//
// Created by chris on 1/6/2025.
//
#include "Parser.h"
#include <cstring>
#include <stdexcept>

Parser::Parser(FILE* inputSteam, FILE* outputStream, FILE* errorStream) :
        m_inputStream(inputSteam), m_outputStream(outputStream), m_errorStream(errorStream) {};

// parses inputs into appropriate flags.
int8_t Parser::parse(int argc, char** argv) {
    if (m_numFlagGroups == 0) {
        fprintf(stderr, "No flag group present\n");
        return -1;
    }

    if (argc <= 1) {
        fprintf(stderr, "No flag provided\n");
        return -1;
    }

    int argvPos = 1;

    // retrieve flag group and set leader flag
    FlagGroup_s* flagGroup;
    const char* flagGroupName = argv[argvPos];    // The flag group's name should always be the first argument
    bool matchedLeader = false;
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        if (std::strcmp(flagGroupName, m_flagGroups[i].flagGroupName_s) == 0) {
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
        fprintf(stderr, "Leader flag not found\n");
        return -1;
    }

    // if any, go through rest of arguments
    for (; argvPos < argc; ++argvPos) {
        const char* currArg = argv[argvPos];
        bool matched = false;

        // find the matching flag
        for (uint8_t i = 0; i < flagGroup->numFlags_s; ++i) {
            if (std::strcmp(currArg, flagGroup->flags_s[i]->name()) == 0) {
                matched = true;

                int8_t parseResult;
                // these branches don't represent SimpleFlag or ArgumentFlag
                if (argvPos + 1 >= argc || argv[argvPos + 1][0] == '-') {
                    parseResult = flagGroup->flags_s[i]->parse(nullptr);
                } else {
                    parseResult = flagGroup->flags_s[i]->parse(argv[argvPos + 1]);
                    argvPos++;
                }

                if (parseResult < 0) {
                    return -1;
                }
            }

            if (matched) {
                break;
            }
        }

        if (!matched) {
            fprintf(stderr, "Unknown flag: %s\n", currArg);
            return -1;
        }
    }

    // verify all required flags are set for this FlagGroup_s
    // **note**: conditional requirements are not checked for   //@TODO: Perhaps implement a system for this?
    return flagGroup->verifyFlags();
}

int8_t Parser::parse(char* input) {
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
    while(*p && *p != target) {
        p++;
    }

    if (*p) {
        *p = '\0';
        p++;
    }

    return p;
}

void Parser::printHelp() const {
    // loop through each FlagGroup_s
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        m_flagGroups[i].printHelp();
        printf("\n");
    }
}

int8_t Parser::FlagGroup_s::verifyFlags() {
    for (uint8_t i = 0; i < this->numFlags_s; ++i) {
        if (!this->flags_s[i]->verify()) {
            fprintf(stderr, "Missing required argument: %s\n", this->flags_s[i]->name());
            return -1;
        }
    }

    return 0;
}

void Parser::resetFlags() {
    // loop through each FlagGroup_s
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        // reset each flag group
        m_flagGroups[i].resetFlags();
    }

    m_latestFlagGroup = -1;
}

int8_t Parser::getFlagGroup(const char* flagGroupName, Parser::FlagGroup_s** flagGroup) {
    for (int i = 0; i < m_numFlagGroups; ++i) {
        if (std::strcmp(flagGroupName, m_flagGroups[i].flagGroupName_s) == 0) {
            *flagGroup = &m_flagGroups[i];
            return 0;
        }
    }

    return -1;
}

Parser::FlagGroup_s* Parser::getFlagGroup(int8_t uid) {
    for (int i = 0; i < m_numFlagGroups; ++i) {
        if (m_flagGroups[i].uid_s == uid) {
            return &m_flagGroups[i];
        }
    }

    return nullptr;
}

int8_t Parser::runFlags() {
    // retrieve the most recent flag group
    FlagGroup_s *flagGroup = getFlagGroup(m_latestFlagGroup);
    if (! flagGroup) {
        return -1;
    }

    flagGroup->runFlags();
    return 0;
}

/* /////////////// */
/* / FlagGroup_s / */
/* /////////////// */

Parser::FlagGroup_s::FlagGroup_s(BaseFlag* flags[], const char* flagGroupName, uint8_t numFlags,
                                 FILE* inputStream, FILE* outputStream, FILE* errorStream,
                                 int8_t uid)
        : flagGroupName_s(flagGroupName), numFlags_s(numFlags),
          inputStream_s(inputStream), outputStream_s(outputStream), errorStream_s(errorStream),
          uid_s(uid) {
    // check flag count
    if (numFlags > MAX_FLAGS) {
        throw std::invalid_argument("Maximum flag count exceeded");
    }

    // copy flags into flags_s
    std::copy(flags, flags + numFlags, flags_s);

    // set each flag's streams
    for (uint8_t i = 0; i < numFlags_s; ++i) {
        flags_s[i]->setStreams(inputStream_s, outputStream_s, errorStream_s);
    }
}

BaseFlag* Parser::FlagGroup_s::getLeader() {
    return flags_s[0];
}

int8_t Parser::FlagGroup_s::getFlag(const char* flagName, BaseFlag** flag) {
    for (int i = 0; i < numFlags_s; ++i) {
        if (std::strcmp(flagName, flags_s[i]->name()) == 0) {
            *flag = flags_s[i];
            return 0;
        }
    }

    return -1;
}

void Parser::FlagGroup_s::printHelp() const {
    // loop through each set of flags within a FlagGroup_s
    for (uint8_t i = 0; i < numFlags_s; ++i) {
        fprintf(outputStream_s, "%s [%s]: %s\n", flags_s[i]->name(), flags_s[i]->isRequired() ? "Required" : "Optional", flags_s[i]->help());
    }
}

void Parser::FlagGroup_s::resetFlags() {
    for (uint8_t i = 0; i < numFlags_s; ++i) {
        flags_s[i]->reset();
    }
}

int8_t Parser::FlagGroup_s::runFlags() {

    for (uint8_t i = 0; i < numFlags_s; ++i) {
        if (flags_s[i]->isSet()) flags_s[i]->run(uid_s);
    }

    return 0;
}
