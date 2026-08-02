#include "ExpressionStore.h"

#include <cstdlib>
#include <cstring>

#include "TriggerVariables.h"

void ExpressionStore::reset() {
    for (uint8_t id = 0; id < m_highestUsed; ++id) {
        destroySlot(id);
    }
    m_highestUsed = 0;
}

uint8_t ExpressionStore::findFreeId() const {
    for (uint8_t i = 0; i < MAX_EXPRESSIONS; ++i) {
        if (m_slots[i].node == nullptr) {
            return i;
        }
    }
    return INVALID_ID;
}

void ExpressionStore::destroySlot(uint8_t id) {
    Expression* node = m_slots[id].node;
    if (node == nullptr) {
        return;
    }
    node->~Expression(); // Virtual dtor -> the correct derived destructor runs.
    m_pool.release(node);
    m_slots[id].node = nullptr;
    m_slots[id].ownerId = INVALID_OWNER;
}

void ExpressionStore::removeOwner(uint8_t ownerId) {
    for (uint8_t id = 0; id < m_highestUsed; ++id) {
        if (m_slots[id].node != nullptr && m_slots[id].ownerId == ownerId) {
            destroySlot(id);
        }
    }
}

void ExpressionStore::tick(const RocketState_s& state) {
    for (uint8_t id = 0; id < m_highestUsed; ++id) {
        if (m_slots[id].node != nullptr) {
            m_slots[id].node->evaluate(state, *this);
        }
    }
}

bool ExpressionStore::getBooleanValue(uint8_t id) const {
    if (id >= m_highestUsed || m_slots[id].node == nullptr) {
        return false;
    }
    return m_slots[id].node->getBooleanValue();
}

float ExpressionStore::getNumberValue(uint8_t id) const {
    if (id >= m_highestUsed || m_slots[id].node == nullptr) {
        return 0.0f;
    }
    return m_slots[id].node->getNumberValue();
}

void ExpressionStore::conditionToString(uint8_t id, char* buffer, int n) const {
    if (id >= m_highestUsed || m_slots[id].node == nullptr) {
        if (n > 0) buffer[0] = '\0';
        return;
    }
    m_slots[id].node->toString(buffer, n, *this);
}

ExpressionValueType_e ExpressionStore::compile(uint8_t ownerId, const char* conditionText, uint8_t* rootIdOut) {
    uint8_t allocated[MAX_EXPRESSIONS];
    uint8_t allocatedCount = 0;
    uint8_t rootId = INVALID_ID;

    const TextSpan_s span{conditionText, conditionText + strlen(conditionText)};
    const ExpressionValueType_e type = parseSpan(span, allocated, allocatedCount, &rootId, 0);

    if (type != ExpressionValueType_e::Boolean) {
        // Roll back whatever this attempt allocated, in reverse (allocation order is always
        // child-before-parent, so this frees parents before the children they reference).
        for (int i = static_cast<int>(allocatedCount) - 1; i >= 0; --i) {
            destroySlot(allocated[i]);
        }
        return ExpressionValueType_e::Invalid;
    }

    removeOwner(ownerId); // Old tree is only torn down once the replacement fully parsed.
    for (uint8_t i = 0; i < allocatedCount; ++i) {
        m_slots[allocated[i]].ownerId = ownerId;
    }
    *rootIdOut = rootId;
    return type;
}

ExpressionValueType_e ExpressionStore::parseSpan(TextSpan_s span, uint8_t* allocated, uint8_t& allocatedCount,
                                                  uint8_t* idOut, int depth) {
    if (depth > MAX_PARSE_DEPTH) {
        return ExpressionValueType_e::Invalid;
    }
    span = trimSpan(span);
    if (span.length() == 0) {
        return ExpressionValueType_e::Invalid;
    }

    bool hadParens = false;
    const TextSpan_s inner = stripOuterParens(span, &hadParens);
    if (hadParens) {
        TextSpan_s tokens[3];
        const int tokenCount = splitTopLevel(inner, tokens, 3);
        if (tokenCount == 2) {
            return parseUnary(tokens[0], tokens[1], allocated, allocatedCount, idOut, depth + 1);
        }
        if (tokenCount == 3) {
            return parseBinary(tokens[0], tokens[1], tokens[2], allocated, allocatedCount, idOut, depth + 1);
        }
        return ExpressionValueType_e::Invalid;
    }

    // Not parenthesized: an atomic token - a numeric constant or a variable name. Neither can
    // legally contain a space, so treating all of `span` as one token here is correct.
    char token[32];
    copyToken(span, token, sizeof(token));

    char* endPtr = nullptr;
    const float value = strtof(token, &endPtr);
    if (endPtr != token && *endPtr == '\0') {
        const uint8_t id = allocateNode<ConstExpression>(allocated, allocatedCount, value);
        if (id == INVALID_ID) return ExpressionValueType_e::Invalid;
        *idOut = id;
        return ExpressionValueType_e::Number;
    }

    const TriggerVariable_e variable = getTriggerVariableFromString(token);
    if (variable != TriggerVariable_e::Invalid) {
        const uint8_t id = allocateNode<VarExpression>(allocated, allocatedCount, variable);
        if (id == INVALID_ID) return ExpressionValueType_e::Invalid;
        *idOut = id;
        return ExpressionValueType_e::Number;
    }

    return ExpressionValueType_e::Invalid;
}

ExpressionValueType_e ExpressionStore::parseUnary(TextSpan_s funcSpan, TextSpan_s operandSpan, uint8_t* allocated,
                                                   uint8_t& allocatedCount, uint8_t* idOut, int depth) {
    char funcName[16];
    copyToken(funcSpan, funcName, sizeof(funcName));

    uint8_t operandId;
    const ExpressionValueType_e operandType = parseSpan(operandSpan, allocated, allocatedCount, &operandId, depth);
    if (operandType == ExpressionValueType_e::Invalid) {
        return ExpressionValueType_e::Invalid;
    }

    for (uint8_t i = 0; i < static_cast<uint8_t>(UnaryFunction_e::NumUnaryFunctions); ++i) {
        const UnaryFunctionInfo_s& info = UNARY_FUNCTIONS[i];
        if (strcmp(info.notation, funcName) == 0 && info.operandType == operandType) {
            const uint8_t id =
                allocateNode<UnaryFuncExpression>(allocated, allocatedCount, static_cast<UnaryFunction_e>(i), operandId);
            if (id == INVALID_ID) return ExpressionValueType_e::Invalid;
            *idOut = id;
            return info.resultType;
        }
    }
    return ExpressionValueType_e::Invalid;
}

ExpressionValueType_e ExpressionStore::parseBinary(TextSpan_s operand1Span, TextSpan_s funcSpan,
                                                    TextSpan_s operand2Span, uint8_t* allocated,
                                                    uint8_t& allocatedCount, uint8_t* idOut, int depth) {
    char funcName[16];
    copyToken(funcSpan, funcName, sizeof(funcName));

    uint8_t operand1Id;
    const ExpressionValueType_e operand1Type =
        parseSpan(operand1Span, allocated, allocatedCount, &operand1Id, depth);
    if (operand1Type == ExpressionValueType_e::Invalid) {
        return ExpressionValueType_e::Invalid;
    }

    uint8_t operand2Id;
    const ExpressionValueType_e operand2Type =
        parseSpan(operand2Span, allocated, allocatedCount, &operand2Id, depth);
    if (operand2Type == ExpressionValueType_e::Invalid) {
        return ExpressionValueType_e::Invalid;
    }

    for (uint8_t i = 0; i < static_cast<uint8_t>(BinaryFunction_e::NumBinaryFunctions); ++i) {
        const BinaryFunctionInfo_s& info = BINARY_FUNCTIONS[i];
        if (strcmp(info.notation, funcName) == 0 && info.operand1Type == operand1Type &&
            info.operand2Type == operand2Type) {
            const uint8_t id = allocateNode<BinaryFuncExpression>(allocated, allocatedCount,
                                                                   static_cast<BinaryFunction_e>(i), operand1Id,
                                                                   operand2Id);
            if (id == INVALID_ID) return ExpressionValueType_e::Invalid;
            *idOut = id;
            return info.resultType;
        }
    }
    return ExpressionValueType_e::Invalid;
}
