#ifndef TRIGGER_EXPRESSION_PROTOTYPE_UNARYFUNCEXPRESSION_H
#define TRIGGER_EXPRESSION_PROTOTYPE_UNARYFUNCEXPRESSION_H

#include <cstdint>

#include "Expression.h"

/// Opcodes for every unary function a trigger condition can use, e.g. `(not launched)`.
enum class UnaryFunction_e : uint8_t {
    Not = 0,
    Ever,
    Always,
    Sin,
    Cos,
    Tan,
    Abs,
    NumUnaryFunctions
};

using UnaryFunctionImpl = void (*)(Expression& self, const Expression& operand, uint32_t nowMs);

/**
 * @brief Everything needed to parse, evaluate, and print one unary function: its notation name
 * (e.g. "not"), the value type it requires of its operand, the value type it produces, and (for
 * boolean-latching functions like "ever"/"always") the value to seed a fresh node with before its
 * first real evaluate() call.
 */
struct UnaryFunctionInfo_s {
    const char* notation;
    UnaryFunctionImpl function;
    ExpressionValueType_e operandType;
    ExpressionValueType_e resultType;
    bool latchSeed;
};

extern const UnaryFunctionInfo_s UNARY_FUNCTIONS[static_cast<uint8_t>(UnaryFunction_e::NumUnaryFunctions)];

/**
 * @class UnaryFuncExpression
 * @brief A node applying one named unary function (e.g. "not", "ever") to a single operand node.
 */
class UnaryFuncExpression : public Expression {
public:
    UnaryFuncExpression(UnaryFunction_e opcode, uint8_t operandId);

    void evaluate(const RocketState_s& state, const ExpressionStore& store) override;

    int toString(char* buffer, int n, const ExpressionStore& store) const override;

private:
    UnaryFunction_e m_opcode;
    uint8_t m_operandId;
};

#endif //TRIGGER_EXPRESSION_PROTOTYPE_UNARYFUNCEXPRESSION_H
