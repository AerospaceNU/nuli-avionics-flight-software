#ifndef DESKTOP_SIMPLEFLAG_H
#define DESKTOP_SIMPLEFLAG_H


#include "BaseFlag.h"

/**
 * @class SimpleFlag
 * @brief Derived class of BaseFlag. Defines boolean flags.
 * @details This class is derived from BaseFlag and implements a basic boolean
 * flag. A boolean flag is either set or unset.
 */
class SimpleFlag : public BaseFlag {
public:
    /**
     * @brief Constructor for a SimpleFlag
     * @param name Name of the flag
     * @param helpText Name, or calling sign, of the flag
     * @param m_required If a flag is required
     */
    SimpleFlag(const char* name, const char* helpText, bool m_required, uint8_t uid, void (*callback)(uint8_t*, uint32_t length, uint8_t, uint8_t));

    /**
     * @brief Retrieves the flag's name
     * @return the flag's name
     */
    const char* name() const override;

    /**
     * @brief Retrieves the help text
     * @return the help text, a `const char*`
     */
    const char* help() const override;

    /**
     * @brief Parses a single flag of the input.
     * @details for SimpleFlag, the argument is not needed
     * @param arg argument to parse into flag, is nullable
     * @return 0 if success, negative for failure
     */
    int8_t parse(char* arg) override;

    /**
     * @brief Dispatches to a pre-set m_callback function.
     */
    void run(uint8_t groupUid) override;

    /**
     * @brief Tells the caller if this flag has been set.
     * @return true if set
     */
    bool isSet() const override;

    /**
     * @brief Tells the caller if this flag is required.
     * @return true if required
     */
    bool isRequired() const override;

    /**
     * @brief Resets all dynamic parameters of flag
     */
    void reset() override;

    /**
     * @brief ensures a flags parameters are correctly set
     * @return true if successful
     */
    bool verify() const override;

    /**
     * @brief Retrieves the value of a derived flag
     * @tparam T type of the value
     * @return A flag's value
     */
    bool getValueDerived() const;

protected:
    /**
    * @brief Retrieves the flag of a flag from a derived class
    * @param outValue Output
    */
    void getValueRaw(void* outValue) const override;
};


#endif //DESKTOP_SIMPLEFLAG_H
