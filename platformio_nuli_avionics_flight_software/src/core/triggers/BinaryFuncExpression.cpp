#include "BinaryFuncExpression.h"

#include <cmath>
#include <cstdio>

#include "ExpressionStore.h"

namespace {

void andFn(Expression& self, const Expression& a, const Expression& b, uint32_t now) {
    self.setBooleanValue(a.getBooleanValue() && b.getBooleanValue(), now);
}

void orFn(Expression& self, const Expression& a, const Expression& b, uint32_t now) {
    self.setBooleanValue(a.getBooleanValue() || b.getBooleanValue(), now);
}

void leqFn(Expression& self, const Expression& a, const Expression& b, uint32_t now) {
    self.setBooleanValue(a.getNumberValue() <= b.getNumberValue(), now);
}

void geqFn(Expression& self, const Expression& a, const Expression& b, uint32_t now) {
    self.setBooleanValue(a.getNumberValue() >= b.getNumberValue(), now);
}

void ltFn(Expression& self, const Expression& a, const Expression& b, uint32_t now) {
    self.setBooleanValue(a.getNumberValue() < b.getNumberValue(), now);
}

void gtFn(Expression& self, const Expression& a, const Expression& b, uint32_t now) {
    self.setBooleanValue(a.getNumberValue() > b.getNumberValue(), now);
}

void eqFn(Expression& self, const Expression& a, const Expression& b, uint32_t now) {
    self.setBooleanValue(std::fabs(a.getNumberValue() - b.getNumberValue()) < 0.001f, now);
}

// "(<seconds> after <condition>)" - true once `now` is more than <seconds> past when <condition>
// FIRST became true, regardless of whether it's still true.
void afterFn(Expression& self, const Expression& a, const Expression& b, uint32_t now) {
    const auto conditionMs = static_cast<uint32_t>(a.getNumberValue() * 1000.0f);
    const uint32_t firstTrue = b.firstTrueMs();
    self.setBooleanValue(firstTrue != Expression::UNSET_MS && (now - firstTrue) > conditionMs, now);
}

// "(<condition> for <seconds>)" - true while <condition> has been continuously true for at least
// <seconds>.
void forFn(Expression& self, const Expression& a, const Expression& b, uint32_t now) {
    const auto conditionMs = static_cast<uint32_t>(b.getNumberValue() * 1000.0f);
    const uint32_t trueSince = a.trueSinceMs();
    self.setBooleanValue(
        trueSince != Expression::UNSET_MS && a.getBooleanValue() && (now - trueSince) > conditionMs, now);
}

void addFn(Expression& self, const Expression& a, const Expression& b, uint32_t /*now*/) {
    self.setNumberValue(a.getNumberValue() + b.getNumberValue());
}

void subFn(Expression& self, const Expression& a, const Expression& b, uint32_t /*now*/) {
    self.setNumberValue(a.getNumberValue() - b.getNumberValue());
}

void mulFn(Expression& self, const Expression& a, const Expression& b, uint32_t /*now*/) {
    self.setNumberValue(a.getNumberValue() * b.getNumberValue());
}

void divFn(Expression& self, const Expression& a, const Expression& b, uint32_t /*now*/) {
    self.setNumberValue(a.getNumberValue() / b.getNumberValue());
}

} // namespace

const BinaryFunctionInfo_s BINARY_FUNCTIONS[static_cast<uint8_t>(BinaryFunction_e::NumBinaryFunctions)] = {
    {"and", andFn, ExpressionValueType_e::Boolean, ExpressionValueType_e::Boolean, ExpressionValueType_e::Boolean},
    {"or", orFn, ExpressionValueType_e::Boolean, ExpressionValueType_e::Boolean, ExpressionValueType_e::Boolean},
    {"<=", leqFn, ExpressionValueType_e::Number, ExpressionValueType_e::Number, ExpressionValueType_e::Boolean},
    {">=", geqFn, ExpressionValueType_e::Number, ExpressionValueType_e::Number, ExpressionValueType_e::Boolean},
    {"<", ltFn, ExpressionValueType_e::Number, ExpressionValueType_e::Number, ExpressionValueType_e::Boolean},
    {">", gtFn, ExpressionValueType_e::Number, ExpressionValueType_e::Number, ExpressionValueType_e::Boolean},
    {"==", eqFn, ExpressionValueType_e::Number, ExpressionValueType_e::Number, ExpressionValueType_e::Boolean},
    {"after", afterFn, ExpressionValueType_e::Number, ExpressionValueType_e::Boolean, ExpressionValueType_e::Boolean},
    {"for", forFn, ExpressionValueType_e::Boolean, ExpressionValueType_e::Number, ExpressionValueType_e::Boolean},
    {"+", addFn, ExpressionValueType_e::Number, ExpressionValueType_e::Number, ExpressionValueType_e::Number},
    {"-", subFn, ExpressionValueType_e::Number, ExpressionValueType_e::Number, ExpressionValueType_e::Number},
    {"*", mulFn, ExpressionValueType_e::Number, ExpressionValueType_e::Number, ExpressionValueType_e::Number},
    {"/", divFn, ExpressionValueType_e::Number, ExpressionValueType_e::Number, ExpressionValueType_e::Number},
};

BinaryFuncExpression::BinaryFuncExpression(BinaryFunction_e opcode, uint8_t operand1Id, uint8_t operand2Id)
    : m_opcode(opcode), m_operand1Id(operand1Id), m_operand2Id(operand2Id) {}

void BinaryFuncExpression::evaluate(const RocketState_s& state, const ExpressionStore& store) {
    const BinaryFunctionInfo_s& info = BINARY_FUNCTIONS[static_cast<uint8_t>(m_opcode)];
    info.function(*this, store.at(m_operand1Id), store.at(m_operand2Id), state.timestamp.runtime_ms);
}

int BinaryFuncExpression::toString(char* buffer, int n, const ExpressionStore& store) const {
    char left[64] = {};
    char right[64] = {};
    store.at(m_operand1Id).toString(left, sizeof(left), store);
    store.at(m_operand2Id).toString(right, sizeof(right), store);
    const BinaryFunctionInfo_s& info = BINARY_FUNCTIONS[static_cast<uint8_t>(m_opcode)];
    return snprintf(buffer, static_cast<size_t>(n), "(%s %s %s)", left, info.notation, right);
}
