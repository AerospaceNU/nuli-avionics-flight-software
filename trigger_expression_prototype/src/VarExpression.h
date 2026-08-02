#ifndef TRIGGER_EXPRESSION_PROTOTYPE_VAREXPRESSION_H
#define TRIGGER_EXPRESSION_PROTOTYPE_VAREXPRESSION_H

#include "Expression.h"
#include "TriggerVariables.h"

/**
 * @class VarExpression
 * @brief A leaf node that evaluates to the current value of one RocketState_s field.
 * @details This is the only expression type that reads live rocket state - every other node
 * either holds a fixed value or combines other nodes' values.
 */
class VarExpression : public Expression {
public:
    explicit VarExpression(TriggerVariable_e variable);

    void evaluate(const RocketState_s& state, const ExpressionStore& store) override;

    int toString(char* buffer, int n, const ExpressionStore& store) const override;

private:
    TriggerVariable_e m_variable;
};

#endif //TRIGGER_EXPRESSION_PROTOTYPE_VAREXPRESSION_H
