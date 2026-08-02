#include "Expression.h"

bool Expression::getBooleanValue() const {
    return m_type == ExpressionValueType_e::Boolean && m_value.boolean;
}

float Expression::getNumberValue() const {
    return m_type == ExpressionValueType_e::Number ? m_value.number : 0.0f;
}

void Expression::setNumberValue(float value) {
    m_type = ExpressionValueType_e::Number;
    m_value.number = value;
}

void Expression::setBooleanValue(bool value, uint32_t nowMs) {
    const bool wasTrue = getBooleanValue();
    m_type = ExpressionValueType_e::Boolean;
    if (m_trueSinceMs == UNSET_MS || (!wasTrue && value) || !value) {
        m_trueSinceMs = nowMs;
    }
    m_value.boolean = value;
    if (value && m_firstTrueMs == UNSET_MS) {
        m_firstTrueMs = nowMs;
    }
}

void Expression::seedBooleanValue(bool value) {
    m_type = ExpressionValueType_e::Boolean;
    m_value.boolean = value;
}
