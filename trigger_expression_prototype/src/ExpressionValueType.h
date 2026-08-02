#ifndef TRIGGER_EXPRESSION_PROTOTYPE_EXPRESSIONVALUETYPE_H
#define TRIGGER_EXPRESSION_PROTOTYPE_EXPRESSIONVALUETYPE_H

/**
 * @brief The kind of value an Expression node evaluates to.
 * @details Every function (unary/binary) declares which of these it accepts for each operand and
 * which it produces, so a trigger condition can be type-checked once at parse time (e.g. rejecting
 * `(altitudeM and 3)`, since `and` only accepts BOOLEAN operands) instead of at every tick.
 */
enum class ExpressionValueType_e {
    Invalid,
    Boolean,
    Number
};

#endif //TRIGGER_EXPRESSION_PROTOTYPE_EXPRESSIONVALUETYPE_H
