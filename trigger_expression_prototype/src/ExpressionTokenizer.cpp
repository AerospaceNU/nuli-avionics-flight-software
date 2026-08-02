#include "ExpressionTokenizer.h"

#include <cstring>

TextSpan_s trimSpan(TextSpan_s span) {
    while (span.begin < span.end && *span.begin == ' ') ++span.begin;
    while (span.end > span.begin && *(span.end - 1) == ' ') --span.end;
    return span;
}

TextSpan_s stripOuterParens(TextSpan_s span, bool* hadParens) {
    span = trimSpan(span);
    *hadParens = false;
    if (span.length() < 2 || *span.begin != '(' || *(span.end - 1) != ')') {
        return span;
    }

    int depth = 0;
    for (const char* p = span.begin; p < span.end; ++p) {
        if (*p == '(') {
            ++depth;
        } else if (*p == ')') {
            --depth;
            if (depth == 0 && p != span.end - 1) {
                return span; // Closed early - e.g. "(a) and (b)" - not one wrapping group.
            }
        }
    }

    *hadParens = true;
    return {span.begin + 1, span.end - 1};
}

int splitTopLevel(TextSpan_s span, TextSpan_s* tokens, int maxTokens) {
    span = trimSpan(span);
    int count = 0;
    int depth = 0;
    const char* tokenStart = span.begin;

    for (const char* p = span.begin; p <= span.end; ++p) {
        const bool atEnd = (p == span.end);
        if (!atEnd) {
            if (*p == '(') ++depth;
            else if (*p == ')') --depth;
        }

        if (depth == 0 && (atEnd || *p == ' ')) {
            if (p > tokenStart) {
                if (count >= maxTokens) return -1;
                tokens[count++] = trimSpan({tokenStart, p});
            }
            tokenStart = p + 1;
        }
    }
    return count;
}

void copyToken(TextSpan_s span, char* buffer, size_t bufferSize) {
    if (bufferSize == 0) return;
    size_t len = span.length();
    if (len >= bufferSize) len = bufferSize - 1;
    memcpy(buffer, span.begin, len);
    buffer[len] = '\0';
}
