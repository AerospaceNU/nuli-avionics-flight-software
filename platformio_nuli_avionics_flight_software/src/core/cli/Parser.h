#ifndef DESKTOP_PARSER_H
#define DESKTOP_PARSER_H

#include "BaseFlag.h"

/**
 * @TODO: Formalize error handling. Currently, some errors are printed at their
 *          source, while in other places, they are printed out by the parent
 *          method.
 *
 * @TODO: create a help-command that prints all flags with their set value
 *
 * @TODO: increase validation.
 *          ex: no duplicate Leader flags
 *          ex: no duplicate flags within a FlagGroup
 *          ex: co-dependencies (this might be a nice to add)
 *          ex: flag uid
 *
 * @TODO: Observe memory usage between `template<uint8_t n>` and passing in `n`
 *          Perhaps get rid of the template for addFlagGroup because right now
 *          the compiler would create a new instance for every new `n`
 */

const uint8_t MAX_FLAG_GROUPS = 16;    ///< Maximum number of FlagGroups
const uint8_t MAX_FLAGS = 16;          ///< Maximum number of flags per FlagGroup

/**
 * @class Parser
 * @brief Parses command line inputs into pre-defined FlagGroups and Flags
 * @details This class parses a user's input, either from the command-line
 * arguments or from input from stdin into FlagGroups containing sets of flags.
 * A user should use the addFlagGroup method to add sets of flags to a group.
 * \n\n
 * A FlagGroup is defined as a set of Flags. The first flag in a FlagGroup is
 * defined as a FlagGroup's "leader". Among FlagGroups, only the leader flag
 * must be unique, collision is permitted for members of a single FlagGroup
 * with another members of a different FlagGroup.
 */
class Parser {
public:
    /**
     * @brief Default constructor
     */
    Parser() = default;

    /**
     * @brief Default destructor
     */
    ~Parser() = default;

    /**
     * @brief Adds a set of Flags into a FlagGroup
     * @details Creates a new FlagGroup
     * @tparam n Number of flags provided (not user inputted)
     * @param flagGroup An array of flags
     * @return 0 if successful
     */
    template<uint8_t n>
    CLIReturnCode_e addFlagGroup(BaseFlag* (&flagGroup)[n]);

    /**
     * @brief Adds a set of Flags into a FlagGroup
     * @details Creates a new FlagGroup
     * @tparam n Number of flags provided (not user inputted)
     * @param flagGroup An array of flags
     * @param uid a unique identifier for the flagGroup
     * @return 0 if successful
     */
    template<uint8_t n>
    CLIReturnCode_e addFlagGroup(BaseFlag* (&flagGroup)[n], int8_t uid);

    /**
     * @brief Parses program argument inputs into FlagGroups
     * @details Alternative to <code>parse(char* input)</code>. Useful in cases
     * where the user is taking inputs directly from program arguments.
     * @param argc Number of arguments
     * @param argv char* array of arguments
     * @return 0 if successful
     */
    CLIReturnCode_e parse(int argc, char* argv[]);

    /**
     * @brief Parses command line input into FlagGroups
     * @details Alternative to <code>parse(int argc, char* argv[])</code>.
     * Useful in cases where the user wants to take a char* input. This method
     * simply parses the input into a form usable by the aforementioned parse
     * method then passes the appropriate arguments to that method.
     * @param input a char* stream
     * @return 0 if successful
     */
    CLIReturnCode_e parse(char* input);

    /**
     * @brief Retrieves a flag's value given its name and its flag group name
     * @tparam T Type of the flag
     * @param flagGroupName Name of its flag group
     * @param flagName Name of the flag
     * @param value location to store value returned
     * @return 0 if successful
     */
    template<typename T>
    CLIReturnCode_e getValue(const char* flagGroupName, const char* flagName, T &value);

    /**
     * @brief Runs the set of flags most recently passed in
     * @details Uses the provided m_callback functions to run the flags which
     * were most recently passed in.
     * @return 0 if successful
     */
    CLIReturnCode_e runFlags();

    /**
     * @brief Prints help text for each FlagGroup
     */
    void printHelp() const;

    /**
     * @brief Resets all flags their default states for each FlagGroup
     * @details Changes a flag's state to before it was parsed. This does
     * not effect core parameters such as a flag's name or help text. This
     * only changes parameters that were set after Parser.parse(...) was
     * called.
     */
    void resetFlags();

protected:
private:
    /**
     * @struct FlagGroup_s
     * @brief Represents a set of Flags
     * @details This is an internal organization tool and should not be exposed
     * to the user. Provides functionality to interact with flags within group.
     */
    struct FlagGroup_s {
        /**
         * @brief Default constructor
         * @warning Constructor for internal use only
         */
        FlagGroup_s() : flags_s{nullptr}, flagGroupName_s{nullptr}, numFlags_s(0),
                        uid_s(-1) {}

        /**
         * @brief Constructor
         * @param flags A constant size array of flags
         * @param flagGroupName The reference for this FlagGroup
         * @param numFlags The number of flags added
         */
        FlagGroup_s(BaseFlag* flags[], const char* flagGroupName, uint8_t numFlags,
                    int8_t uid);

        /**
         * @brief Retrieves the leader's flag
         * @return A BaseFlag pointer to the leader
         */
        BaseFlag* getLeader();

        /**
         * @brief Retrieves specified flag from group
         * @param flagName The desired flag
         * @param flag Returned flag
         * @return 0 if success
         */
        CLIReturnCode_e getFlag(const char* flagName, BaseFlag** flag);

        /**
         * @brief Ensures all member flag parameters are correctly set
         * @return 0 if successful
         */
        CLIReturnCode_e verifyFlags();

        /**
         * @brief Runs the callback functions of the most recent flag group set
         */
        void runFlags();

        /**
         * @brief Compares two strings for equivalency
         * @details Both strings must be valid cstrings
         * @param string1
         * @param string2
         * @return
         */
        int strcmp(const char* string1, const char* string2);

        /**
         * @brief Prints help text for each Flag
         */
        void printHelp() const;

        /**
         * @brief Resets all flags within the group to their default states
         * @details Changes a flag's state to before it was parsed. This does
         * not effect core parameters such as a flag's name or help text. This
         * only changes parameters that were set after Parser.parse(...) was
         * called.
         */
        void resetFlags();

        BaseFlag* flags_s[MAX_FLAGS] = {nullptr};   ///< The flags within this FlagGroup
        const char* flagGroupName_s = {nullptr};    ///< The leader flag's name
        uint8_t numFlags_s;                         ///< number of flags within FlagGroup
        int8_t uid_s;                               ///< unique number identifying FlagGroup
    };


    /**
     * @brief Helper function for parse
     * @param p Pointer to the input character array
     * @param target The target character (e.g. a quotation mark or space)
     * @return A pointer to the end of the segment
     */
    char* getString(char* p, char target) const;

    /**
     * @brief Compares two strings for equivalency
     * @details Both strings must be valid cstrings
     * @param string1
     * @param string2
     * @return
     */
    int strcmp(const char* string1, const char* string2);

    /**
     * @brief Retrieves a flag group based on its name
     * @param flagGroupName Name of the desired flag group
     * @param flagGroup Placeholder
     * @return 0 if success
     */
    CLIReturnCode_e getFlagGroup(const char* flagGroupName, FlagGroup_s** flagGroup);

    /**
     * @brief Retrieves a flag group based on its identification number
     * @param uid a (hopefully) unique number identifying the flag group
     * @return 0 if success
     */
    CLIReturnCode_e getFlagGroup(int8_t uid, FlagGroup_s** flagGroup);

    /**
     * @brief Identifies if an argument belongs to a FlagGroup
     * @param arg Argument
     * @param flagGroupS FlagGroup which arg might belong to
     * @return true if argument is in flag group
     */
    bool isKnownFlag(char* arg, FlagGroup_s& flagGroupS);

    FlagGroup_s m_flagGroups[MAX_FLAG_GROUPS];  ///< FlagGroups
    uint8_t m_numFlagGroups = 0;                ///< number of FlagGroups
    int8_t m_uid = 0;                           ///< index of uids
    int8_t m_latestFlagGroup = -1;              ///< the last flag group processed
};

#include "Parser.tpp"

#endif //DESKTOP_PARSER_H
