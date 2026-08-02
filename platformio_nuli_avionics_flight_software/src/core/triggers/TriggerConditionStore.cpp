#include "TriggerConditionStore.h"

#include <cstring>

#include "util/CRC.h"

void TriggerConditionStore::setup(FramMemory* memory, DebugStream* debugStream) {
    m_memory = memory;

    uint32_t storedCrc = 0;
    m_memory->read(FRAM_OFFSET, reinterpret_cast<uint8_t*>(&storedCrc), sizeof(storedCrc));
    m_memory->read(FRAM_OFFSET + sizeof(storedCrc), reinterpret_cast<uint8_t*>(&m_data), sizeof(m_data));

    if (storedCrc != calculateCrc()) {
        debugStream->warn("Trigger condition store invalid CRC, resetting all triggers to disabled");
        m_data = StoredData_s{};
        m_dirty = true;
        pushUpdatesToMemory();
    }
}

const char* TriggerConditionStore::getCondition(uint8_t triggerId) const {
    if (triggerId >= MAX_TRIGGERS) return "";
    return m_data.conditions[triggerId];
}

void TriggerConditionStore::setCondition(uint8_t triggerId, const char* condition) {
    if (triggerId >= MAX_TRIGGERS) return;
    strncpy(m_data.conditions[triggerId], condition, CONDITION_LEN - 1);
    m_data.conditions[triggerId][CONDITION_LEN - 1] = '\0';
    m_dirty = true;
}

void TriggerConditionStore::pushUpdatesToMemory() {
    if (!m_dirty) return;
    const uint32_t crc = calculateCrc();
    m_memory->write(FRAM_OFFSET, reinterpret_cast<const uint8_t*>(&crc), sizeof(crc));
    m_memory->write(FRAM_OFFSET + sizeof(crc), reinterpret_cast<const uint8_t*>(&m_data), sizeof(m_data));
    m_dirty = false;
}

uint32_t TriggerConditionStore::calculateCrc() const {
    return crc32(&m_data, sizeof(m_data));
}
