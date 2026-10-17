// Throwaway test harness for BasicLoggerHelper.h
#pragma once
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#define remove_struct_padding __attribute__((packed))
#include "core/BasicLoggerHelper.h"

struct Cap : DebugStream {
    std::string s;
    size_t write(const void* b, size_t n) override {
        s.append((const char*)b, n);
        return n;
    }
};

constexpr bool streq(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

using Rng = std::mt19937_64;

struct Stats {
    long rows = 0, fields = 0, failures = 0;
    long doubleRoundTripMiss = 0, negZeroTokens = 0;
    double worstDoubleRelErr = 0;
    size_t longestFloatTok = 0;
    long expTokens = 0;
    long dblMaxReadsAsInf = 0;
};
extern Stats g;

inline void fail(const char* where, const std::string& msg) {
    if (++g.failures <= (getenv("ALLFAIL") ? 100000000 : 40)) printf("FAIL [%s] %s\n", where, msg.c_str());
}

// ---------- random values ----------
template <class T>
typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, T>::type randVal(Rng& r) {
    typedef std::numeric_limits<T> L;
    switch (r() % 10) {
    case 0: return L::min();
    case 1: return L::max();
    case 2: return 0;
    case 3: return (T)(L::min() + 1);
    case 4: return (T)(L::max() - 1);
    case 5: return (T)-1;
    case 6: return L::is_signed ? (T)((long long)(r() % 21) - 10) : (T)(r() % 21); // small values
    default: {
        uint64_t bits = r();
        T v;
        memcpy(&v, &bits, sizeof(T));
        return v;
    }
    }
}

template <class T>
typename std::enable_if<std::is_same<T, bool>::value, T>::type randVal(Rng& r) { return r() & 1; }

inline double pow10i(int e) { return std::pow(10.0, e); }

template <class T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type randVal(Rng& r) {
    typedef std::numeric_limits<T> L;
    double sign = (r() & 1) ? -1.0 : 1.0;
    switch (r() % 16) {
    case 0: return L::quiet_NaN();
    case 1: return (T)(sign * L::infinity());
    case 2: return (T)(sign * 0.0);
    case 3: return (T)(sign * L::denorm_min());
    case 4: return (T)(sign * L::min());
    case 5: return (T)(sign * L::max());
    case 6: { // just around powers of ten / digit boundaries
        T base = (T)pow10i((int)(r() % 40) - 20);
        int steps = (int)(r() % 7) - 3;
        T v = base;
        for (int i = 0; i < std::abs(steps); i++) v = std::nextafter(v, steps < 0 ? (T)0 : L::infinity());
        return (T)(sign * v);
    }
    case 7: { // x.9999... style values that round up
        int e = (int)(r() % 20) - 10;
        return (T)(sign * (pow10i(e) - pow10i(e - 9) * 0.4));
    }
    case 8: return (T)(sign * (double)(r() % 100000)); // integers
    case 9: return (T)(sign * pow10i(19) * (0.5 + (r() % 1000) / 1000.0)); // near ovf boundary
    case 10: { // raw bit patterns
        T v;
        if (sizeof(T) == 4) { uint32_t b = (uint32_t)r(); memcpy(&v, &b, 4); }
        else { uint64_t b = r(); memcpy(&v, &b, 8); }
        return v;
    }
    default: { // log-uniform magnitude 1e-15 .. 1e21
        double e = -15.0 + (r() % 3600000) / 100000.0;
        return (T)(sign * std::pow(10.0, e));
    }
    }
}

// ---------- reference checks ----------
template <class T>
typename std::enable_if<std::is_integral<T>::value>::type check(const std::string& tok, T v, const char* name) {
    std::string exp = std::is_signed<T>::value ? std::to_string((long long)v) : std::to_string((unsigned long long)v);
    size_t maxLen = std::max(std::to_string((long long)std::numeric_limits<T>::min()).size(),
                             std::is_signed<T>::value ? std::to_string((long long)std::numeric_limits<T>::max()).size()
                                                      : std::to_string((unsigned long long)std::numeric_limits<T>::max()).size());
    if (tok.size() > maxLen) fail(name, "int token longer than type's widest value");
    if (tok != exp) fail(name, "int: expected '" + exp + "' got '" + tok + "'");
}

inline bool wellFormedNumber(const std::string& t) {
    size_t i = 0;
    if (i < t.size() && t[i] == '-') i++;
    size_t digitsStart = i;
    while (i < t.size() && isdigit((unsigned char)t[i])) i++;
    size_t intLen = i - digitsStart;
    if (intLen == 0) return false;
    if (intLen > 1 && t[digitsStart] == '0') return false; // leading zero
    if (i == t.size()) return true;
    if (t[i] != '.') return false;
    i++;
    size_t fracStart = i;
    while (i < t.size() && isdigit((unsigned char)t[i])) i++;
    if (i != t.size() || i == fracStart) return false;
    return t.back() != '0'; // trailing zeros must be stripped
}

template <class T>
typename std::enable_if<std::is_floating_point<T>::value>::type check(const std::string& tok, T v, const char* name) {
    const bool isFloat = sizeof(T) == 4;
    const int sig = isFloat ? 9 : 17;
    char vbuf[64];
    snprintf(vbuf, sizeof vbuf, "%.17g", (double)v);
    std::string ctx = std::string(" value=") + vbuf + " tok='" + tok + "'";
    if (tok.size() > (isFloat ? 21u : 23u)) fail(name, "token longer than worst-case bound" + ctx);
    if (tok.size() > g.longestFloatTok) g.longestFloatTok = tok.size();
    if (std::isnan(v)) { if (tok != "nan") fail(name, "nan" + ctx); return; }
    if (std::isinf(v)) { if (tok != (v < 0 ? "-inf" : "inf")) fail(name, "inf" + ctx); return; }
    double a = std::fabs((double)v);
    if (a >= 1e19) { // exponent form: <fixed mantissa>e<exp>
        size_t e = tok.find('e');
        if (e == std::string::npos || !wellFormedNumber(tok.substr(0, e)) || e + 1 >= tok.size() ||
            tok.find_first_not_of("0123456789", e + 1) != std::string::npos) { fail(name, "bad exponent form" + ctx); return; }
        double p = strtod(tok.c_str(), nullptr);
        if (isFloat) { if ((float)p != (float)v) fail(name, "float exp round-trip" + ctx); }
        else if (std::isinf(p) && a > DBL_MAX * (1 - 1e-15)) g.dblMaxReadsAsInf++; // documented limitation
        else if (std::fabs(p - (double)v) / a > 1e-14) fail(name, "double exp rel err > 1e-14" + ctx);
        g.expTokens++;
        return;
    }
    if (!wellFormedNumber(tok)) { fail(name, "malformed" + ctx); return; }
    if (tok == "-0") g.negZeroTokens++;
    double p = strtod(tok.c_str(), nullptr);
    if ((p < 0) != (v < 0) && p != 0) { fail(name, "sign" + ctx); return; }
    // significant digit budget
    size_t digits = 0, intDigits = 0;
    bool seenDot = false;
    for (char c : tok) {
        if (c == '.') seenDot = true;
        else if (isdigit((unsigned char)c)) { digits++; if (!seenDot) intDigits++; }
    }
    if (a >= 1) {
        size_t budget = intDigits > (size_t)sig ? intDigits : (size_t)sig;
        if (digits > budget) fail(name, "too many digits" + ctx);
        if (isFloat) {
            if ((float)p != (float)v) fail(name, "float round-trip" + ctx);
        } else if (p != (double)v) {
            g.doubleRoundTripMiss++;
            double rel = std::fabs(p - v) / a;
            if (rel > g.worstDoubleRelErr) g.worstDoubleRelErr = rel;
            if (rel > 1e-15) fail(name, "double rel err > 1e-15" + ctx);
        }
    } else {
        double allowed = 0.5 * pow10i(-sig) + 8 * DBL_EPSILON * a + 1e-300;
        if (std::fabs(p - (double)v) > allowed) fail(name, "abs err below 1" + ctx);
    }
}

inline std::vector<std::string> splitTabs(const std::string& line) {
    std::vector<std::string> out;
    size_t start = 0;
    for (;;) {
        size_t t = line.find('\t', start);
        out.push_back(line.substr(start, t == std::string::npos ? std::string::npos : t - start));
        if (t == std::string::npos) break;
        start = t + 1;
    }
    return out;
}
