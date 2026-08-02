#ifndef TRIGGER_EXPRESSION_PROTOTYPE_EXPRESSIONTOKENIZER_H
#define TRIGGER_EXPRESSION_PROTOTYPE_EXPRESSIONTOKENIZER_H

#include <cstddef>

/**
 * @brief A [begin, end) view into a caller-owned string.
 * @details Used while parsing a trigger's notation string so sub-expressions can be identified
 * without copying, until a token is short enough to need null-terminating for a lookup (see
 * copyToken()).
 */
struct TextSpan_s {
    const char* begin;
    const char* end;

    size_t length() const { return static_cast<size_t>(end - begin); }
};

/// Trims leading/trailing spaces from `span`.
TextSpan_s trimSpan(TextSpan_s span);

/**
 * @brief If `span` is wrapped in one matching pair of parentheses (e.g. "(a and b)"), returns the
 * span with those parens stripped (e.g. "a and b") and sets `hadParens` true. Otherwise returns
 * `span` unchanged (trimmed) and sets `hadParens` false.
 * @details Checks that the opening paren's matching close is the span's last character, so
 * something like "(a) and (b)" - which merely starts and ends with parens without being one
 * wrapped group - is correctly left alone.
 */
TextSpan_s stripOuterParens(TextSpan_s span, bool* hadParens);

/**
 * @brief Splits `span` on top-level spaces (i.e. not inside a nested parenthesized group) into at
 * most `maxTokens` sub-spans.
 * @return The number of tokens found, or -1 if there were more than maxTokens.
 */
int splitTopLevel(TextSpan_s span, TextSpan_s* tokens, int maxTokens);

/// Copies `span` into `buffer` as a null-terminated C string, truncating to fit `bufferSize`.
void copyToken(TextSpan_s span, char* buffer, size_t bufferSize);

#endif //TRIGGER_EXPRESSION_PROTOTYPE_EXPRESSIONTOKENIZER_H
