#include "UnaryFuncExpression.h"

#include <cmath>
#include <cstdio>

#include "ExpressionStore.h"

namespace {

void notFn(Expression& self, const Expression& op, uint32_t now) {
    self.setBooleanValue(!op.getBooleanValue(), now);
}

// "ever <condition>" latches true forever once <condition> has been true at least once.
void everFn(Expression& self, const Expression& op, uint32_t now) {
    self.setBooleanValue(self.getBooleanValue() || op.getBooleanValue(), now);
}

// "always <condition>" latches false forever the first time <condition> is false.
void alwaysFn(Expression& self, const Expression& op, uint32_t now) {
    self.setBooleanValue(self.getBooleanValue() && op.getBooleanValue(), now);
}

void sinFn(Expression& self, const Expression& op, uint32_t /*now*/) {
    self.setNumberValue(sinf(op.getNumberValue()));
}

void cosFn(Expression& self, const Expression& op, uint32_t /*now*/) {
    self.setNumberValue(cosf(op.getNumberValue()));
}

void tanFn(Expression& self, const Expression& op, uint32_t /*now*/) {
    self.setNumberValue(tanf(op.getNumberValue()));
}

void absFn(Expression& self, const Expression& op, uint32_t /*now*/) {
    self.setNumberValue(std::fabs(op.getNumberValue()));
}

} // namespace

const UnaryFunctionInfo_s UNARY_FUNCTIONS[static_cast<uint8_t>(UnaryFunction_e::NumUnaryFunctions)] = {
    {"not", notFn, ExpressionValueType_e::Boolean, ExpressionValueType_e::Boolean, false},
    {"ever", everFn, ExpressionValueType_e::Boolean, ExpressionValueType_e::Boolean, false},
    {"always", alwaysFn, ExpressionValueType_e::Boolean, ExpressionValueType_e::Boolean, true},
    {"sin", sinFn, ExpressionValueType_e::Number, ExpressionValueType_e::Number, false},
    {"cos", cosFn, ExpressionValueType_e::Number, ExpressionValueType_e::Number, false},
    {"tan", tanFn, ExpressionValueType_e::Number, ExpressionValueType_e::Number, false},
    {"abs", absFn, ExpressionValueType_e::Number, ExpressionValueType_e::Number, false},
};

UnaryFuncExpression::UnaryFuncExpression(UnaryFunction_e opcode, uint8_t operandId)
    : m_opcode(opcode), m_operandId(operandId) {
    const UnaryFunctionInfo_s& info = UNARY_FUNCTIONS[static_cast<uint8_t>(opcode)];
    if (info.resultType == ExpressionValueType_e::Boolean) {
        seedBooleanValue(info.latchSeed);
    }
}

void UnaryFuncExpression::evaluate(const RocketState_s& state, const ExpressionStore& store) {
    const UnaryFunctionInfo_s& info = UNARY_FUNCTIONS[static_cast<uint8_t>(m_opcode)];
    info.function(*this, store.at(m_operandId), state.timestamp.runtime_ms);
}

int UnaryFuncExpression::toString(char* buffer, int n, const ExpressionStore& store) const {
    char operand[64] = {};
    store.at(m_operandId).toString(operand, sizeof(operand), store);
    const UnaryFunctionInfo_s& info = UNARY_FUNCTIONS[static_cast<uint8_t>(m_opcode)];
    return snprintf(buffer, static_cast<size_t>(n), "(%s %s)", info.notation, operand);
}
