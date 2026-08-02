#ifndef EXPRESSIONSTORE_H
#define EXPRESSIONSTORE_H

#include <cstdint>

#include <etl/generic_pool.h>

#include "Avionics.h"
#include "BinaryFuncExpression.h"
#include "ConstExpression.h"
#include "EmptyExpression.h"
#include "Expression.h"
#include "ExpressionTokenizer.h"
#include "ExpressionValueType.h"
#include "UnaryFuncExpression.h"
#include "VarExpression.h"

namespace expression_store_detail {
// Not std::max(initializer_list) - not constexpr in every libstdc++ this project's embedded
// toolchains ship (fine under desktop GCC 13, but not arm-none-eabi-g++ 9.3.1's older one). A free
// function rather than a static member of ExpressionStore itself, because arm-none-eabi-g++ 9.3.1
// also rejects a class's own static constexpr member calling a same-class static member function
// from within that class's still-incomplete definition.
static constexpr size_t constexprMax(size_t a, size_t b) { return a > b ? a : b; }
} // namespace expression_store_detail

/**
 * @class ExpressionStore
 * @brief Owns every live Expression node backing every configured pyro trigger.
 * @details Nodes are allocated from a fixed-capacity etl::generic_pool (no heap use once
 * running - the same pattern this codebase already vendors ETL for elsewhere), indexed by a
 * uint8_t id so a trigger config can cheaply reference "its" root node.
 *
 * compile() only replaces a given owner's tree if the replacement notation string parses
 * completely and evaluates to a boolean: parsing happens into fresh slots first, and the owner's
 * old nodes are only freed once that fully succeeds. A mistyped CLI edit can never leave a
 * trigger half-configured or silently disable a working one.
 */
class ExpressionStore {
public:
    static constexpr uint8_t MAX_EXPRESSIONS = 32;
    static constexpr uint8_t INVALID_ID = 0xFF;
    static constexpr uint8_t INVALID_OWNER = 0xFF;
    static constexpr int MAX_PARSE_DEPTH = 8;

    ExpressionStore() = default;
    ExpressionStore(const ExpressionStore&) = delete;
    ExpressionStore& operator=(const ExpressionStore&) = delete;
    ~ExpressionStore() { reset(); }

    /// @brief Frees every node owned by every trigger. Leaves the store as if freshly constructed.
    void reset();

    /**
     * @brief Parses `conditionText` and, only if it parses completely to a boolean expression,
     * replaces whatever tree `ownerId` previously owned with the new one.
     * @param ownerId Caller-defined tag (e.g. a pyro/trigger channel index) used to later free
     * this tree's nodes via removeOwner(). This store places no bound on the value beyond it
     * fitting in a uint8_t.
     * @param conditionText Notation string, e.g. "((altitudeM < 50) and (velocityMS < 0))".
     * @param rootIdOut Set to the id of the root node on success; untouched on failure.
     * @return The root's value type (always Boolean on success - a trigger condition must
     * resolve to a boolean); Invalid if `conditionText` didn't parse, overflowed the pool, or
     * parsed to a number instead of a boolean.
     */
    ExpressionValueType_e compile(uint8_t ownerId, const char* conditionText, uint8_t* rootIdOut);

    /// @brief Frees every node owned by `ownerId`. A no-op if it owns nothing.
    void removeOwner(uint8_t ownerId);

    /// @brief Re-evaluates every live node, in ascending id order. Node ids are always allocated
    /// child-before-parent (see compile()), so this always evaluates a node's operands before
    /// the node that reads them.
    void tick(const RocketState_s& state);

    bool getBooleanValue(uint8_t id) const;

    float getNumberValue(uint8_t id) const;

    /// @brief Renders the tree rooted at `id` back into notation string form.
    void conditionToString(uint8_t id, char* buffer, int n) const;

    /// @brief Resolves a node id to the node itself. Used by Unary/BinaryFuncExpression to reach
    /// their operands, and internally while rendering toString(). Undefined behavior if `id` was
    /// never returned by compile() for a still-live tree.
    Expression& at(uint8_t id) const { return *m_slots[id].node; }

private:
    static constexpr size_t SLOT_SIZE = expression_store_detail::constexprMax(
        expression_store_detail::constexprMax(
            expression_store_detail::constexprMax(sizeof(ConstExpression), sizeof(VarExpression)),
            sizeof(UnaryFuncExpression)),
        expression_store_detail::constexprMax(sizeof(BinaryFuncExpression), sizeof(EmptyExpression)));
    static constexpr size_t SLOT_ALIGN = expression_store_detail::constexprMax(
        expression_store_detail::constexprMax(
            expression_store_detail::constexprMax(alignof(ConstExpression), alignof(VarExpression)),
            alignof(UnaryFuncExpression)),
        expression_store_detail::constexprMax(alignof(BinaryFuncExpression), alignof(EmptyExpression)));

    struct Slot_s {
        Expression* node = nullptr;
        uint8_t ownerId = INVALID_OWNER;
    };

    uint8_t findFreeId() const;

    void destroySlot(uint8_t id);

    template <typename T, typename... Args>
    uint8_t allocateNode(uint8_t* allocated, uint8_t& allocatedCount, Args&&... args);

    // --- Recursive-descent parsing. Each level allocates its operand(s) before itself, so within
    // one compile() call, node ids always come out child-before-parent (see findFreeId()). All
    // take `depth` and fail closed (return Invalid) past MAX_PARSE_DEPTH, bounding recursion on
    // pathological/malicious input (e.g. deeply-nested parens).
    ExpressionValueType_e parseSpan(TextSpan_s span, uint8_t* allocated, uint8_t& allocatedCount, uint8_t* idOut,
                                     int depth);
    ExpressionValueType_e parseUnary(TextSpan_s funcSpan, TextSpan_s operandSpan, uint8_t* allocated,
                                      uint8_t& allocatedCount, uint8_t* idOut, int depth);
    ExpressionValueType_e parseBinary(TextSpan_s operand1Span, TextSpan_s funcSpan, TextSpan_s operand2Span,
                                       uint8_t* allocated, uint8_t& allocatedCount, uint8_t* idOut, int depth);

    etl::generic_pool<SLOT_SIZE, SLOT_ALIGN, MAX_EXPRESSIONS> m_pool;
    Slot_s m_slots[MAX_EXPRESSIONS];
    uint8_t m_highestUsed = 0; ///< One past the highest id ever allocated; bounds tick()/reset() scans.
};

#include "ExpressionStore.tpp"

#endif //EXPRESSIONSTORE_H
