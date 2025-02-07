//
// Created by chris on 1/6/2025.
//

#ifndef DESKTOP_PARSER_H
#define DESKTOP_PARSER_H
#include "Flag.h"

/**
 * @TODO: Formalize error handling. Currently, some errors are printed at their
 *          source, while in other places, they are printed out by the parent
 *          method.
 *
 * @TODO: Formalize handling of template methods. Currently, sometimes they are
 *          1. put in the header file with the declaration, 2. defined at the
 *          bottom of the header file, 3. in a separate .tpp file.
 *
 * @TODO: create a side-command that prints all flags with their set value
 *
 * @TODO: increase validation.
 *          ex: no duplicate Leader flags
 *          ex: no duplicate flags within a FlagGroup
 *          ex: co-dependencies (this might be a nice to add)
 */

const uint8_t MAX_FLAG_GROUPS = 255;    ///< Maximum number of FlagGroups
const uint8_t MAX_FLAGS = 255;          ///< Maximum number of flags per FlagGroup

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
    int8_t addFlagGroup(BaseFlag* (&flagGroup)[n]) {
        // bounds checks
        if (n == 0) {
            fprintf(stderr, "No flag group provided\n");
            return -1;
        }

        if (m_numFlagGroups > MAX_FLAG_GROUPS) {
            fprintf(stderr, "Maximum flag groups exceeded\n");
            return -1;
        }

        FlagGroup_s newFlagGroup(flagGroup, flagGroup[0]->name(), n);
        m_flagGroups[m_numFlagGroups++] = newFlagGroup;

        return 0;
    }

    /**
     * @brief Parses program argument inputs into FlagGroups
     * @details Alternative to <code>parse(char* input)</code>. Useful in cases
     * where the user is taking inputs directly from program arguments.
     * @param argc Number of arguments
     * @param argv char* array of arguments
     * @return 0 if successful
     */
    int8_t parse(int argc, char* argv[]);

    /**
     * @brief Parses command line input into FlagGroups
     * @details Alternative to <code>parse(int argc, char* argv[])</code>.
     * Useful in cases where the user wants to take a char* input. This method
     * simply parses the input into a form usable by the aforementioned parse
     * method then passes the appropriate arguments to that method.
     * @param input a char* stream
     * @return 0 if successful
     */
    int8_t parse(char* input);

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
        FlagGroup_s() : flagGroupName_s{nullptr}, flags_s{nullptr}, numFlags_s(0) {}

        /**
         * @brief Constructor
         * @param flags A constant size array of flags
         * @param flagGroupName The reference for this FlagGroup
         * @param numFlags The number of flags added
         */
        FlagGroup_s(BaseFlag* flags[], const char* flagGroupName, uint8_t numFlags);

        /**
         * @brief Retrieves the leader's flag
         * @return A BaseFlag pointer to the leader
         */
        BaseFlag* getLeader();

        /**
         * @brief Ensures all member flag parameters are correctly set
         * @return 0 if successful
         */
        int8_t verifyFlags();

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

    };

    FlagGroup_s m_flagGroups[MAX_FLAG_GROUPS];  ///< FlagGroups
    uint8_t m_numFlagGroups = 0;                ///< number of FlagGroups
};

#endif //DESKTOP_PARSER_H
