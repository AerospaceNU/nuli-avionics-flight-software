#include "ConstExpression.h"

#include <cstdio>

ConstExpression::ConstExpression(float value) {
    setNumberValue(value);
}

void ConstExpression::evaluate(const RocketState_s& /*state*/, const ExpressionStore& /*store*/) {
    // A constant's value never changes after construction.
}

int ConstExpression::toString(char* buffer, int n, const ExpressionStore& /*store*/) const {
    return snprintf(buffer, static_cast<size_t>(n), "%.2f", static_cast<double>(getNumberValue()));
}
