//
// Created by chris on 1/6/2025.
//
#include "Parser.h"
#include <cstring>
#include <stdexcept>

void Parser::addFlag(BaseFlag& flag) {
    if (m_numFlags < MAX_FLAGS) {
        m_flags[m_numFlags++] = &flag;
    } else {
        throw std::invalid_argument("Too many arguments");
    }
}

void Parser::parse(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const char* currArg = argv[i];
        bool matched = false;
        // finding the flag
        for (uint8_t j = 0; j < m_numFlags; ++j) {
            if (std::strcmp(currArg, m_flags[j]->name()) == 0) {
                matched = true;

                // @TODO: Currently if a flag that requires an argument does not have the required argument, it fails
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    m_flags[j]->parse(argv[++i]);   // pass the next argument as this flag's value
                } else {
                    m_flags[j]->parse(nullptr);
                }

                break;
            }
        }

        // unknown flag
        if (!matched) {
            throw std::invalid_argument("Unknown flag!");
        }
    }

    // verify required flags present
    for (uint8_t i = 0; i < m_numFlags; ++i) {
        if (m_flags[i]->isRequired() && !m_flags[i]->isSet()) {
            throw std::invalid_argument("Missing required argument");
        }
    }
}

void Parser::parse(char* input) {
    // removing '\n'
    input[std::strcspn(input, "\n")] = 0;

    // parsing into argc and argv
    // @TODO: I don't know if setting to 1 is best. Bit of a magic number
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

BaseFlag* Parser::getFlag(const char* flagName) {
    for (uint8_t i = 0; i < m_numFlags; ++i) {
        if (strcmp(m_flags[i]->name(), flagName) == 0) {
            return m_flags[i];
        }
    }

    return nullptr;
}

void Parser::printHelp() const {
    for (size_t i = 0; i < m_numFlags; ++i) {
        printf("%s: %s\n", m_flags[i]->name(), m_flags[i]->help());
    }
}
