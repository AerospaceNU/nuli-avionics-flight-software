#ifndef TRIGGER_EXPRESSION_PROTOTYPE_BINARYFUNCEXPRESSION_H
#define TRIGGER_EXPRESSION_PROTOTYPE_BINARYFUNCEXPRESSION_H

#include <cstdint>

#include "Expression.h"

/// Opcodes for every binary function a trigger condition can use, e.g. `(altitudeM < 50)`.
enum class BinaryFunction_e : uint8_t {
    And = 0,
    Or,
    Leq,
    Geq,
    Lt,
    Gt,
    Eq,
    After,
    For,
    Add,
    Sub,
    Mul,
    Div,
    NumBinaryFunctions
};

using BinaryFunctionImpl = void (*)(Expression& self, const Expression& op1, const Expression& op2, uint32_t nowMs);

/**
 * @brief Everything needed to parse, evaluate, and print one binary function: its notation name
 * (e.g. "and", "<="), the value types it requires of each operand, and the value type it produces.
 */
struct BinaryFunctionInfo_s {
    const char* notation;
    BinaryFunctionImpl function;
    ExpressionValueType_e operand1Type;
    ExpressionValueType_e operand2Type;
    ExpressionValueType_e resultType;
};

extern const BinaryFunctionInfo_s BINARY_FUNCTIONS[static_cast<uint8_t>(BinaryFunction_e::NumBinaryFunctions)];

/**
 * @class BinaryFuncExpression
 * @brief A node combining two operand nodes with one named binary function (e.g. "and", "for").
 */
class BinaryFuncExpression : public Expression {
public:
    BinaryFuncExpression(BinaryFunction_e opcode, uint8_t operand1Id, uint8_t operand2Id);

    void evaluate(const RocketState_s& state, const ExpressionStore& store) override;

    int toString(char* buffer, int n, const ExpressionStore& store) const override;

private:
    BinaryFunction_e m_opcode;
    uint8_t m_operand1Id;
    uint8_t m_operand2Id;
};

#endif //TRIGGER_EXPRESSION_PROTOTYPE_BINARYFUNCEXPRESSION_H
