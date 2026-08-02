#ifndef CONSTEXPRESSION_H
#define CONSTEXPRESSION_H

#include "Expression.h"

/**
 * @class ConstExpression
 * @brief A leaf node holding a fixed numeric value (e.g. the `50` in `(altitudeM < 50)`).
 */
class ConstExpression : public Expression {
public:
    explicit ConstExpression(float value);

    void evaluate(const RocketState_s& state, const ExpressionStore& store) override;

    int toString(char* buffer, int n, const ExpressionStore& store) const override;
};

#endif //CONSTEXPRESSION_H
