#ifndef DESKTOP_PARSER_H
#define DESKTOP_PARSER_H

#include "BaseFlag.h"

const uint8_t MAX_FLAG_GROUPS = 30;    ///< Keep headroom above the busiest board's registered group count
const uint8_t MAX_FLAGS = 8;            ///< Per FlagGroup

/**
 * @brief Parses input into registered FlagGroups; the first flag of a group is its leader.
 * @details Only leader names must be unique across groups; member flag names may repeat between groups.
 */
class Parser {
public:
    Parser() = default;

    ~Parser() = default;

    template<uint8_t n>
    CLIReturnCode_e addFlagGroup(BaseFlag* (&flagGroup)[n]);

    CLIReturnCode_e parse(int argc, char* argv[]);

    /**
     * @brief Tokenizes input in place, then forwards to parse(argc, argv).
     */
    CLIReturnCode_e parse(char* input);

    template<typename T>
    CLIReturnCode_e getValue(const char* flagGroupName, const char* flagName, T &value);

    /**
     * @brief Runs the callbacks of the most recently parsed FlagGroup.
     */
    CLIReturnCode_e runFlags(DebugStream* debugStream);

    void printHelp(DebugStream* debugStream) const;

    /**
     * @brief Returns every flag to its pre-parse state.
     */
    void resetFlags();

protected:
private:
    struct FlagGroup_s {
        FlagGroup_s() : flags_s{nullptr}, flagGroupName_s{nullptr}, numFlags_s(0),
                        uid_s(-1) {}

        FlagGroup_s(BaseFlag* flags[], const char* flagGroupName, uint8_t numFlags,
                    int8_t uid);

        BaseFlag* getLeader();

        CLIReturnCode_e getFlag(const char* flagName, BaseFlag** flag);

        CLIReturnCode_e verifyFlags();

        void runFlags(DebugStream* debugStream);

        int strcmp(const char* string1, const char* string2);

        void printHelp(DebugStream* debugStream) const;

        void resetFlags();

        BaseFlag* flags_s[MAX_FLAGS] = {nullptr};
        const char* flagGroupName_s = {nullptr};    ///< The leader flag's name
        uint8_t numFlags_s;
        int8_t uid_s;
    };


    /**
     * @brief Null-terminates the token starting at p (ends at target or line end); returns the next position.
     */
    char* getString(char* p, char target) const;

    int strcmp(const char* string1, const char* string2);

    CLIReturnCode_e getFlagGroup(const char* flagGroupName, FlagGroup_s** flagGroup);

    CLIReturnCode_e getFlagGroup(int8_t uid, FlagGroup_s** flagGroup);

    bool isKnownFlag(char* arg, FlagGroup_s& flagGroupS);

    FlagGroup_s m_flagGroups[MAX_FLAG_GROUPS];
    uint8_t m_numFlagGroups = 0;
    int8_t m_uid = 0;                           ///< Next uid to assign
    int8_t m_latestFlagGroup = -1;              ///< uid of the last parsed group, -1 if none
};

#include "Parser.tpp"

#endif //DESKTOP_PARSER_H
