#ifndef TRIGGER_EXPRESSION_PROTOTYPE_EXPRESSIONSTORE_TPP
#define TRIGGER_EXPRESSION_PROTOTYPE_EXPRESSIONSTORE_TPP

#include <utility>

template <typename T, typename... Args>
uint8_t ExpressionStore::allocateNode(uint8_t* allocated, uint8_t& allocatedCount, Args&&... args) {
    const uint8_t id = findFreeId();
    if (id == INVALID_ID) {
        return INVALID_ID;
    }
    T* obj = m_pool.create<T>(std::forward<Args>(args)...);
    if (obj == nullptr) {
        return INVALID_ID;
    }
    m_slots[id].node = obj;
    m_slots[id].ownerId = INVALID_OWNER; // Not committed to an owner until compile() fully succeeds.
    if (static_cast<uint8_t>(id + 1) > m_highestUsed) {
        m_highestUsed = static_cast<uint8_t>(id + 1);
    }
    allocated[allocatedCount++] = id;
    return id;
}

#endif //TRIGGER_EXPRESSION_PROTOTYPE_EXPRESSIONSTORE_TPP
