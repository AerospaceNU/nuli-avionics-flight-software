#include "VarExpression.h"

#include <cstring>

VarExpression::VarExpression(TriggerVariable_e variable) : m_variable(variable) {}

void VarExpression::evaluate(const RocketState_s& state, const ExpressionStore& /*store*/) {
    setNumberValue(getTriggerVariableValue(state, m_variable));
}

int VarExpression::toString(char* buffer, int n, const ExpressionStore& /*store*/) const {
    fillTriggerVariableName(buffer, n, m_variable);
    return static_cast<int>(strnlen(buffer, static_cast<size_t>(n)));
}
