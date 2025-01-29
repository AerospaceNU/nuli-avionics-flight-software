//
// Created by chris on 1/6/2025.
//
#include "Parser.h"
#include <cstring>
#include <stdexcept>

// parses inputs into appropriate flags.
void Parser::parse(int argc, char** argv) {
    if (m_numFlagGroups == 0) {
        throw std::runtime_error("No flag groups present");
    }

    if (argc <= 1) {
        throw std::invalid_argument("No flag provided");
    }

    int argvPos = 1;

    // retrieve flag group and set primary flag
    FlagGroup* flagGroup;
    const char* flagGroupName = argv[argvPos++];    // The flag group's name should always be the first argument
    bool matchedPrimary = false;
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        if (std::strcmp(flagGroupName, m_flagGroups[i].flagGroupName_s) == 0) {
            // flag group has been identified
            matchedPrimary = true;
            flagGroup = &m_flagGroups[i];

            // set and parse the primary flag
            BaseFlag* primaryFlag = flagGroup->getPrimary();
            primaryFlag->parse(argc, argv, argvPos);
        }
    }

    if (! matchedPrimary) {
        throw std::invalid_argument("Primary flag not found!"); //@TODO: Change naming to "leader flag"
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
        if (! matched) {
            throw std::invalid_argument("Unknown flag!");
        }
    }

    // verify all required flags are set for this FlagGroup
    // **note**: conditional requirements are not checked for   //@TODO: Perhaps implement a system for this?
    flagGroup->verifyFlags();
}

void Parser::parse(char* input) {
    // removing '\n'
    input[std::strcspn(input, "\n")] = 0;

    // parsing into argc and argv
    int argc = 1;   // compatibility with command-line argc/argv
    char *argv[255] = {nullptr};
    char *savePtr;

    char *p = strtok_r(input, " ", &savePtr);   // strtok is disgusting
    while (p && argc < 255 - 1) {
        argv[argc++] = p;
        p = strtok_r(nullptr, " ", &savePtr);
    }

    argv[argc] = nullptr;

    this->parse(argc, argv);
}

void Parser::printHelp() const {
    // loop through each FlagGroup
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        // loop through each set of flags within a FlagGroup
        for (uint8_t j = 0; j < m_flagGroups[i].numFlags_s; ++j) {
            printf("%s: %s\n", m_flagGroups[i].flags_s[j]->name(), m_flagGroups[i].flags_s[j]->help());
        }
        printf("\n");
    }
}


// ///////////////////////////
/* WORKING WITH FLAG GROUPS */
// ///////////////////////////
BaseFlag* Parser::FlagGroup::getPrimary() {
    if (numFlags_s < 1) {
        throw std::runtime_error("No flag groups added");
    }

    return flags_s[0];
}

bool Parser::FlagGroup::verifyFlags() {
    for (uint8_t i = 0; i < this->numFlags_s; ++i) {
        if (! this->flags_s[i]->verify()) {
            throw std::invalid_argument("Missing required argument");
        }
    }

    return true;
}

void Parser::resetFlags() {
    // loop through each FlagGroup
    for (uint8_t i = 0; i < m_numFlagGroups; ++i) {
        // loop through each set of flags within a FlagGroup
        for (uint8_t j = 0; j < m_flagGroups[i].numFlags_s; ++j) {
            m_flagGroups[i].flags_s[j]->resetFlag();
        }
        printf("\n");
    }
}

void Parser::addFlagGroup(Parser::FlagGroup &flagGroup) {
    if (m_numFlagGroups >= MAX_FLAG_GROUPS) {
        throw std::invalid_argument("Max number of flag groups exceeded");
    }

    m_flagGroups[m_numFlagGroups++] = flagGroup;
}
