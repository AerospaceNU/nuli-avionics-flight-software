#ifndef EMPTYEXPRESSION_H
#define EMPTYEXPRESSION_H

#include "Expression.h"

/**
 * @class EmptyExpression
 * @brief Placeholder type used only to reserve a slot's size/alignment in ExpressionStore's pool.
 * @details Never actually constructed into a live slot - ExpressionStore represents "no node
 * here" as a null pointer, not an EmptyExpression instance.
 */
class EmptyExpression : public Expression {
public:
    void evaluate(const RocketState_s& /*state*/, const ExpressionStore& /*store*/) override {}

    int toString(char* buffer, int n, const ExpressionStore& /*store*/) const override {
        if (n > 0) buffer[0] = '\0';
        return 0;
    }
};

#endif //EMPTYEXPRESSION_H
