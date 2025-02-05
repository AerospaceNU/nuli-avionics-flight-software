//
// Created by chris on 1/6/2025.
//

#ifndef DESKTOP_PARSER_H
#define DESKTOP_PARSER_H
#include "Flag.h"

/*
 * Project Requirements:
 * Use C strings
 * Current ideas might be to implement a method where it has a generic commands
 * class. And then when parse is called on it, you add an instance of the
 * commands class in.
 *
--config -t int*1 Configure a trigger with additional flag:                     <--- the below are required (-m -p -d -w -C -D -N)
         -m int*1 Trigger type (required)
                  Type 1 = pyro
                  Type 2 = line cutter
                  Type 3 = digital on (on pyro)
                  Type 4 = digital off (on pyro)
                  Type 5 = pwm (on pyro)
         -p int*1 Pyro num or cut channel (required)
         -d float*1 Duration (required for pyro and pwm)
         -w float*1 Pulse width (required for pwm)
         -C string*1 Configuration using expression notation, required. Must be in quotes
         -D Delete this trigger
         -N Disallow manual triggering
 -e float*1 Configure ground elevation (in meters)                  <--- these are optional
 -r float*1 Configure ground temperature (in celsius)
 -c int*1 Configure radio channel (in multiples of bandwidth), negative numbers allowed
 -h Help for config. Prints current configuration values

--create_flight  Clear state log and move back to preflight

--erase  Fully erases on-board flash

--help  FCB produces standard command line help string

--linecutter -i int*1 Send linecutter cut with given ID (required)
 -c string*1 Send a command (required)

--offload  Offloads the last flight recorded on the board
 -f int*1 Offload a specific flight number off the board
 -h Help for offload. Prints info about each flight

--triggerfire -t int*1 IMMEDIATELY fires the given trigger number

--sense  Reads back most recent sensor data

--sim  Simulate past flights in hardware

--version  Send Git version and tag info
 */

const uint8_t MAX_INPUT = 64;
const uint8_t MAX_FLAGS = 255;
const uint8_t MAX_FLAG_GROUPS = 255;

/**
 * @class
 * @brief
 */
class Parser {
public:
    /* constructors/destructors */
    Parser() = default;

    ~Parser() = default;

    /* methods */
    template<uint8_t n> // @TODO: Three options. 1: put in .tpp file. 2. put in header with declaration. 3. put definition at bottom of header
    void addFlagGroup(BaseFlag* (&flagGroup)[n]) {
        // bounds checks
        if (n == 0) {
            throw std::invalid_argument("No flag group provided");
        }

        if (m_numFlagGroups > MAX_FLAG_GROUPS) {
            throw std::invalid_argument("Maximum flag groups exceeded");
        }

        FlagGroup_s newFlagGroup(flagGroup, flagGroup[0]->name(), n);
        m_flagGroups[m_numFlagGroups++] = newFlagGroup;
    }

    int8_t parse(int argc, char* argv[]);

    int8_t parse(char* input);

    void printHelp() const;

    void resetFlags();

protected:
private:
    struct FlagGroup_s {
        BaseFlag* flags_s[MAX_FLAGS] = {nullptr};
        const char* flagGroupName_s = {nullptr};
        uint8_t numFlags_s;

        FlagGroup_s() : flagGroupName_s{nullptr}, flags_s{}, numFlags_s(0) {}

        FlagGroup_s(BaseFlag* flags[], const char* flagGroupName, uint8_t numFlags);

        BaseFlag* getLeader();

        int8_t verifyFlags();

        void resetFlags();
    };

    FlagGroup_s m_flagGroups[MAX_FLAG_GROUPS];
    uint8_t m_numFlagGroups = 0;
};

#endif //DESKTOP_PARSER_H
