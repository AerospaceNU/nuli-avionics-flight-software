#ifndef TRIGGER_EXPRESSION_PROTOTYPE_EXPRESSION_H
#define TRIGGER_EXPRESSION_PROTOTYPE_EXPRESSION_H

#include <cstdint>
#include <limits>

#include "Avionics.h"
#include "ExpressionValueType.h"

class ExpressionStore;

/**
 * @class Expression
 * @brief Base class for one node in a pyro trigger's condition tree.
 * @details A trigger condition (e.g. "((altitudeM < 50) and (velocityMS < 0))") is parsed into a
 * tree of Expression nodes owned by an ExpressionStore. Every node caches the value it last
 * evaluated to, plus the timestamps of when it first/most-recently became true, so that
 * time-aware combinators (`for`, `after`) can be built on top of any boolean-valued node without
 * those combinators needing their own timers.
 */
class Expression {
public:
    virtual ~Expression() = default;

    /**
     * @brief Recompute this node's value from the current rocket state.
     * @details Implementations that reference other nodes (unary/binary functions) resolve their
     * operands through `store`. ExpressionStore::tick() evaluates nodes in ascending id order,
     * and node ids are always allocated child-before-parent, so an operand is guaranteed to have
     * already been evaluated this tick before the node that reads it.
     */
    virtual void evaluate(const RocketState_s& state, const ExpressionStore& store) = 0;

    /**
     * @brief Render this node (and its operands, via `store`) back into notation string form.
     * @return Number of characters written (excluding the null terminator).
     */
    virtual int toString(char* buffer, int n, const ExpressionStore& store) const = 0;

    /// @return This node's cached value as a bool. False if this node isn't boolean-valued.
    bool getBooleanValue() const;

    /// @return This node's cached value as a float. 0 if this node isn't number-valued.
    float getNumberValue() const;

    /// @return The timestamp this node first evaluated true, or UNSET_MS if it never has.
    uint32_t firstTrueMs() const { return m_firstTrueMs; }

    /// @return The start of this node's current true-streak. Meaningless while not currently
    /// true (see setBooleanValue()) - callers combine this with getBooleanValue().
    uint32_t trueSinceMs() const { return m_trueSinceMs; }

    static constexpr uint32_t UNSET_MS = std::numeric_limits<uint32_t>::max();

    // These three are called on `self` by the free functions in the unary/binary function tables
    // (see UnaryFuncExpression.cpp/BinaryFuncExpression.cpp) - not just by Expression's own
    // subclasses - so they have to be public rather than protected.

    void setNumberValue(float value);

    /// @brief Sets this node's boolean value and updates firstTrueMs/trueSinceMs.
    /// @details trueSinceMs marks the start of the current true-streak: it jumps to `nowMs` on
    /// every tick this node is false (so it's only meaningful once the node is true again) and on
    /// a false->true transition, but holds steady across consecutive true ticks.
    void setBooleanValue(bool value, uint32_t nowMs);

    /// @brief Seeds the cached boolean value at construction time, without touching
    /// firstTrueMs/trueSinceMs (those stay UNSET_MS until the first real evaluate() call). Used
    /// by self-referential unary functions ("ever"/"always") that read their own prior value on
    /// their very first tick, before any real evaluate() has run.
    void seedBooleanValue(bool value);

protected:
    Expression() = default;

private:
    ExpressionValueType_e m_type = ExpressionValueType_e::Invalid;

    union {
        bool boolean;
        float number;
    } m_value{};

    uint32_t m_firstTrueMs = UNSET_MS;
    uint32_t m_trueSinceMs = UNSET_MS;
};

#endif //TRIGGER_EXPRESSION_PROTOTYPE_EXPRESSION_H
