//
// Created by chris on 1/6/2025.
//
#include "Parser.h"
#include <cstring>
#include <stdexcept>

//@TODO create a side-command that prints all flags with their set value

// parses inputs into appropriate flags.
int8_t Parser::parse(int argc, char** argv) {
    if (m_numFlagGroups == 0) {
        fprintf(stderr, "No flag group present\n");
        return -1;
        throw std::runtime_error("No flag groups present");
    }

    if (argc <= 1) {
        fprintf(stderr, "No flag provided\n");
        return -1;
        throw std::invalid_argument("No flag provided");
    }

    int argvPos = 1;

    // retrieve flag group and set leader flag
    FlagGroup_s* flagGroup;
    const char* flagGroupName = argv[argvPos++];    // The flag group's name should always be the first argument
    bool matchedLeader = false;
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        if (std::strcmp(flagGroupName, m_flagGroups[i].flagGroupName_s) == 0) {
            // flag group has been identified
            matchedLeader = true;
            flagGroup = &m_flagGroups[i];

            // set and parse the leader flag
            BaseFlag* leaderFlag = flagGroup->getLeader();
            if (!leaderFlag) {
                fprintf(stderr, "Unable to retrieve leader flag\n");
                return -1;
            }

            if (leaderFlag->parse(argc, argv, argvPos) < 0) {
                return -1;
            }
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

                flagGroup->flags_s[i]->parse(argc, argv, argvPos);
            }
        }

        // unknown flag
        if (!matched) {
            fprintf(stderr, "Unknown flag\n");
            return -1;
            throw std::invalid_argument("Unknown flag!");
        }
    }

    // verify all required flags are set for this FlagGroup_s
    // **note**: conditional requirements are not checked for   //@TODO: Perhaps implement a system for this?
    return flagGroup->verifyFlags();
}

int8_t Parser::parse(char* input) {
    // removing '\n'
    input[std::strcspn(input, "\n")] = 0;

    // parsing into argc and argv
    int argc = 1;   // compatibility with command-line argc/argv
    char* argv[255] = {nullptr};
    char* savePtr;

    char* p = strtok_r(input, " ", &savePtr);   // strtok is disgusting
    while (p && argc < 255 - 1) {
        argv[argc++] = p;
        p = strtok_r(nullptr, " ", &savePtr);
    }

    argv[argc] = nullptr;

    return this->parse(argc, argv);
}

void Parser::printHelp() const {
    // loop through each FlagGroup_s
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        // loop through each set of flags within a FlagGroup_s
        for (uint8_t j = 0; j < m_flagGroups[i].numFlags_s; ++j) {
            printf("%s: %s\n", m_flagGroups[i].flags_s[j]->name(), m_flagGroups[i].flags_s[j]->help());
        }
        printf("\n");
    }
}


// ///////////////////////////
/* WORKING WITH FLAG GROUPS */
// ///////////////////////////
BaseFlag* Parser::FlagGroup_s::getLeader() {
    if (numFlags_s < 1) {
        return nullptr;
        throw std::runtime_error("No flag groups added");
    }

    return flags_s[0];
}

int8_t Parser::FlagGroup_s::verifyFlags() {
    for (uint8_t i = 0; i < this->numFlags_s; ++i) {
        if (!this->flags_s[i]->verify()) {
            fprintf(stderr, "Missing required arguments\n");
            return -1;
            throw std::invalid_argument("Missing required argument");
        }
    }

    return 0;
}

void Parser::FlagGroup_s::resetFlags() {
    for (uint8_t i = 0; i < numFlags_s; ++i) {
        flags_s[i]->reset();
    }
}

Parser::FlagGroup_s::FlagGroup_s(BaseFlag* flags[], const char* flagGroupName, uint8_t numFlags)
        : flagGroupName_s(flagGroupName), numFlags_s(numFlags) {
    if (numFlags > MAX_FLAGS) {
        throw std::invalid_argument("Maximum flag count exceeded");
    }

    // copy flags into flags_s
    std::copy(flags, flags + numFlags, flags_s);
}

void Parser::resetFlags() {
    // loop through each FlagGroup_s
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        // reset each flag group
        m_flagGroups[i].resetFlags();
    }
}