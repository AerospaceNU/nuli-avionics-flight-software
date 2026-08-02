#ifndef TRIGGERCONDITIONSTORE_H
#define TRIGGERCONDITIONSTORE_H

#include <cstdint>

#include "Avionics.h"
#include "core/generic_hardware/DebugStream.h"
#include "core/generic_hardware/FramMemory.h"

/**
 * @class TriggerConditionStore
 * @brief Persists every trigger's notation-string condition to FRAM, independently of Configuration.
 * @details Lives in the same FRAM chip Configuration uses, but at its own fixed address range
 * starting right after Configuration's maximum possible extent (FRAM_OFFSET == MAX_CONFIGURATION_LENGTH),
 * so it can never collide with Configuration's region regardless of how many configuration
 * variables a given board actually registers. CRC-guarded the same way Configuration guards its
 * own region: an invalid CRC on boot (corrupt FRAM, or first boot ever) resets every trigger to
 * "" (disabled), never to a garbage condition string.
 * \n\n
 * This class only stores/persists the raw condition strings. Parsing them into a live
 * ExpressionStore is the caller's job (see TriggerConditionStore::getCondition() +
 * ExpressionStore::compile()) - kept separate so this class stays a simple, narrow FRAM record,
 * the same way Configuration itself doesn't know anything about the meaning of its values.
 */
class TriggerConditionStore {
public:
    static constexpr uint8_t MAX_TRIGGERS = 4; ///< Room for a few trigger slots; only one is wired up so far
    static constexpr uint16_t CONDITION_LEN = 100; ///< Matches the ConfigurationString<100> convention used elsewhere (e.g. BOARD_NAME)
    static constexpr uint32_t FRAM_OFFSET = MAX_CONFIGURATION_LENGTH;

    /**
     * @brief Reads the store from FRAM, resetting to all-disabled ("") on a CRC mismatch.
     * @param memory The same FramMemory Configuration uses (see HardwareAbstraction::getFramMemory()).
     * @param debugStream Used only to log a warning on CRC mismatch.
     */
    void setup(FramMemory* memory, DebugStream* debugStream);

    /// @return The condition string for `triggerId`, or "" if out of range.
    const char* getCondition(uint8_t triggerId) const;

    /// @brief Updates `triggerId`'s condition in memory and marks the store dirty. Does not touch
    /// FRAM until pushUpdatesToMemory() is called - callers should validate (e.g. via
    /// ExpressionStore::compile()) before calling this, since this class has no opinion on
    /// whether `condition` is well-formed.
    void setCondition(uint8_t triggerId, const char* condition);

    /// @brief Writes the whole store to FRAM if anything changed since the last call. Cheap to
    /// call every loop, mirroring Configuration::pushUpdatesToMemory().
    void pushUpdatesToMemory();

private:
    struct StoredData_s {
        char conditions[MAX_TRIGGERS][CONDITION_LEN];
    };

    uint32_t calculateCrc() const;

    FramMemory* m_memory = nullptr;
    StoredData_s m_data{};
    bool m_dirty = false;
};

#endif //TRIGGERCONDITIONSTORE_H
