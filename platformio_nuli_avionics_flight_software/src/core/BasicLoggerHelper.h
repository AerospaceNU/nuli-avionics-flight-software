#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BASICLOGGERHELPER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BASICLOGGERHELPER_H

#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "core/generic_hardware/DebugStream.h"

/**
 * Defines a log data struct and keeps a string copy of its definition, so the log header and
 * printer can be generated from it instead of being written out by hand:
 *
 *   LOG_DATA_AUTO_DEFINE(LOG_DEFINITION,
 *       struct MyLogData {
 *           uint32_t timestampMs;
 *           float a, b, c;
 *       } remove_struct_padding;
 *   )
 *   static constexpr const char* LOG_HEADER = BasicLoggerHelper::generateBasicLogHeader<LOG_DEFINITION>();
 *   void printLog(const MyLogData& d, DebugStream* debug) {
 *       BasicLoggerHelper::genericBasicLogDataPrinter<LOG_DEFINITION>(d, debug);
 *   }
 *
 * Fields must be plain numbers: bool, float, double, char/short/int/long/long long in any
 * signed/unsigned spelling, or [u]int8_t..[u]int64_t. The struct must be packed. Anything else
 * fails to compile (static_assert). Requires C++14.
 *
 * Use LOG_DATA_AUTO_DEFINE at file scope (not inside a class or function). Only the size is
 * checked against the struct you print, so keep each printer next to its definition. The
 * printer uses a static buffer per struct type: don't call it for the same type from an ISR.
 */
#define LOG_DATA_AUTO_DEFINE(DEFINITION, ...) \
    __VA_ARGS__                               \
    static constexpr char DEFINITION[] = #__VA_ARGS__;

class BasicLoggerHelper {
public:
    /** @brief Tab-separated field names, built at compile time */
    template <const char* Def>
    static constexpr auto& generateBasicLogHeader() {
        return HeaderOf<Def>::value.str;
    }

    /**
     * @brief Prints one tab-separated row of d's fields. Fails to compile if Def doesn't parse or doesn't match T.
     * @details Fields are read with memcpy because packed fields can be unaligned, which hard-faults the Cortex-M0+.
     */
    template <const char* Def, typename T>
    static void genericBasicLogDataPrinter(const T& d, DebugStream* debug) {
        static_assert(std::is_class<T>::value, "pass the log struct itself (by reference), not a pointer or number");
        static_assert(parsedSize(Def) == (long)sizeof(T), "log struct definition didn't parse (unsupported type? not packed?)");
        static_assert(maxRowLength(Def) <= MAX_ROW_CHARS, "log row could overflow DebugStream's line buffer - split the struct");
        static char buf[maxRowLength(Def) + 1];
        Writer out{buf, sizeof(buf), 0};
        const uint8_t* bytes = (const uint8_t*)&d;
        const Field* fields = FieldsOf<Def>::value.fields;
        for (size_t i = 0; i < FieldsOf<Def>::count; i++) {
            const Field& f = fields[i];
            if (i) out.put('\t');
            if (f.kind == kFloat) {
                float v;
                memcpy(&v, bytes + f.offset, sizeof(v));
                out.putFloat(v, 9);
            } else if (f.kind == kDouble) {
                double v;
                memcpy(&v, bytes + f.offset, sizeof(v));
                out.putFloat(v, 17);
            } else {
                uint64_t u = 0;
                memcpy(&u, bytes + f.offset, f.size);
                if (f.kind == kUnsigned) {
                    out.putUnsigned(u);
                } else {
                    const bool negative = f.size < 8 && (u >> (8 * f.size - 1)) & 1;
                    if (negative) u |= ~0ULL << (8 * f.size);
                    out.putSigned((int64_t)u);
                }
            }
        }
        buf[out.pos] = '\0';
        debug->data("%s", buf);
    }

private:
    static constexpr size_t MAX_ROW_CHARS = 1023;

    enum Kind : uint8_t { kUnsigned, kSigned, kFloat, kDouble };

    struct TypeInfo {
        uint8_t size;
        Kind kind;
    };

    struct Field {
        const char* name;
        size_t nameLen;
        Kind kind;
        uint8_t size;
        size_t offset;
    };

    static constexpr bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

    static constexpr bool isIdent(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    }

    static constexpr const char* skipSpace(const char* p) {
        while (isSpace(*p)) p++;
        return p;
    }

    static constexpr size_t identLen(const char* p) {
        size_t n = 0;
        while (isIdent(p[n])) n++;
        return n;
    }

    static constexpr bool matches(const char* p, size_t n, const char* name) {
        for (size_t i = 0; i < n; i++) {
            if (p[i] != name[i]) return false;
        }
        return name[n] == '\0';
    }

    static constexpr TypeInfo lookupNamedType(const char* p, size_t n) {
        return matches(p, n, "bool")     ? TypeInfo{sizeof(bool), kUnsigned}
             : matches(p, n, "float")    ? TypeInfo{sizeof(float), kFloat}
             : matches(p, n, "double")   ? TypeInfo{sizeof(double), kDouble}
             : matches(p, n, "int8_t")   ? TypeInfo{1, kSigned}
             : matches(p, n, "int16_t")  ? TypeInfo{2, kSigned}
             : matches(p, n, "int32_t")  ? TypeInfo{4, kSigned}
             : matches(p, n, "int64_t")  ? TypeInfo{8, kSigned}
             : matches(p, n, "uint8_t")  ? TypeInfo{1, kUnsigned}
             : matches(p, n, "uint16_t") ? TypeInfo{2, kUnsigned}
             : matches(p, n, "uint32_t") ? TypeInfo{4, kUnsigned}
             : matches(p, n, "uint64_t") ? TypeInfo{8, kUnsigned}
             :                             TypeInfo{0, kUnsigned};
    }

    /** @brief Walks "struct X { type a, b; type c; } ..." one field at a time, assuming the struct is packed */
    struct FieldIterator {
        const char* p;
        TypeInfo type;
        size_t offset;
        bool error;

        constexpr explicit FieldIterator(const char* def) : p(def), type{0, kUnsigned}, offset(0), error(false) {
            while (*p && *p != '{') p++;
            if (*p) p++;
            else error = true;
        }

        constexpr TypeInfo readType() {
            bool isUnsigned = false, isSigned = false, isShort = false, isChar = false, anyKeyword = false;
            int longs = 0;
            for (;;) {
                p = skipSpace(p);
                size_t n = identLen(p);
                if (matches(p, n, "unsigned")) isUnsigned = true;
                else if (matches(p, n, "signed")) isSigned = true;
                else if (matches(p, n, "short")) isShort = true;
                else if (matches(p, n, "long")) longs++;
                else if (matches(p, n, "char")) isChar = true;
                else if (matches(p, n, "int")) {}
                else break;
                anyKeyword = true;
                p += n;
            }
            if (!anyKeyword) {
                size_t n = identLen(p);
                TypeInfo t = lookupNamedType(p, n);
                if (t.size) p += n;
                return t;
            }
            uint8_t size = isChar ? sizeof(char)
                         : isShort ? sizeof(short)
                         : longs == 1 ? sizeof(long)
                         : longs >= 2 ? sizeof(long long)
                         : sizeof(int);
            bool isSignedType = isUnsigned ? false : isSigned ? true : isChar ? ((char)-1 < 0) : true;
            return TypeInfo{size, isSignedType ? kSigned : kUnsigned};
        }

        constexpr bool next(Field& f) {
            if (error) return false;
            p = skipSpace(p);
            if (type.size == 0) {
                if (*p == '}') return false;
                type = readType();
                if (type.size == 0) {
                    error = true;
                    return false;
                }
                p = skipSpace(p);
            }
            size_t n = identLen(p);
            if (n == 0) {
                error = true;
                return false;
            }
            f = Field{p, n, type.kind, type.size, offset};
            offset += type.size;
            p = skipSpace(p + n);
            if (*p == ',') {
                p++;
            } else if (*p == ';') {
                p++;
                type = TypeInfo{0, kUnsigned};
            } else {
                error = true;
                return false;
            }
            return true;
        }
    };

    static constexpr long parsedSize(const char* def) {
        FieldIterator it(def);
        Field f{};
        while (it.next(f)) {}
        return it.error ? -1 : (long)it.offset;
    }

    static constexpr size_t maxFieldChars(const Field& f) {
        return f.kind == kFloat ? 21
             : f.kind == kDouble ? 23
             : f.kind == kUnsigned ? (f.size == 1 ? 3 : f.size == 2 ? 5 : f.size == 4 ? 10 : 20)
             : (f.size == 1 ? 4 : f.size == 2 ? 6 : f.size == 4 ? 11 : 20);
    }

    static constexpr size_t maxRowLength(const char* def) {
        FieldIterator it(def);
        Field f{};
        size_t len = 0;
        bool first = true;
        while (it.next(f)) {
            len += (first ? 0 : 1) + maxFieldChars(f);
            first = false;
        }
        return len;
    }

    static constexpr size_t headerLength(const char* def) {
        FieldIterator it(def);
        Field f{};
        size_t len = 0;
        while (it.next(f)) len += (len ? 1 : 0) + f.nameLen;
        return len;
    }

    template <size_t N>
    struct ConstString {
        char str[N];
    };

    template <size_t N>
    static constexpr ConstString<N> buildHeader(const char* def) {
        ConstString<N> out{};
        size_t pos = 0;
        FieldIterator it(def);
        Field f{};
        while (it.next(f)) {
            if (pos) out.str[pos++] = '\t';
            for (size_t i = 0; i < f.nameLen; i++) out.str[pos++] = f.name[i];
        }
        return out;
    }

    template <const char* Def>
    struct HeaderOf {
        static_assert(parsedSize(Def) >= 0, "log struct definition didn't parse (unsupported type?)");
        using Storage = ConstString<headerLength(Def) + 1>;
        static constexpr Storage value = buildHeader<headerLength(Def) + 1>(Def);
    };

    static constexpr size_t fieldCount(const char* def) {
        FieldIterator it(def);
        Field f{};
        size_t n = 0;
        while (it.next(f)) n++;
        return n;
    }

    template <size_t N>
    struct FieldTable {
        Field fields[N];
    };

    template <size_t N>
    static constexpr FieldTable<N> buildFieldTable(const char* def) {
        FieldTable<N> table{};
        FieldIterator it(def);
        Field f{};
        for (size_t i = 0; i < N && it.next(f); i++) table.fields[i] = f;
        return table;
    }

    /** @brief The definition parsed once at compile time - re-parsing it per row cost a few ms on the SAMD21 */
    template <const char* Def>
    struct FieldsOf {
        static constexpr size_t count = fieldCount(Def) ? fieldCount(Def) : 1;
        using Table = FieldTable<count>;
        static constexpr Table value = buildFieldTable<count>(Def);
    };

    /** @brief Number formatting into a fixed buffer - the SAMD builds' newlib-nano printf has no float support */
    struct Writer {
        char* buf;
        size_t size;
        size_t pos;

        void put(char c) {
            if (pos + 1 < size) buf[pos++] = c;
        }

        void puts(const char* s) {
            while (*s) put(*s++);
        }

        void putUnsigned(uint64_t v) {
            char tmp[20];
            int n = 0;
            if (v <= UINT32_MAX) {
                uint32_t v32 = (uint32_t)v;
                do {
                    tmp[n++] = (char)('0' + v32 % 10);
                    v32 /= 10;
                } while (v32);
            } else {
                do {
                    tmp[n++] = (char)('0' + v % 10);
                    v /= 10;
                } while (v);
            }
            while (n) put(tmp[--n]);
        }

        void putSigned(int64_t v) {
            if (v < 0) {
                put('-');
                putUnsigned(0 - (uint64_t)v);
            } else {
                putUnsigned((uint64_t)v);
            }
        }

        /** @brief sigDigits significant digits (decimal places below 1), trailing zeros stripped, exponent form from 1e19 */
        void putFloat(double v, int sigDigits) {
            if (v != v) {
                puts("nan");
                return;
            }
            const bool negative = v < 0;
            if (negative) v = -v;
            if (v > DBL_MAX) {
                puts(negative ? "-inf" : "inf");
                return;
            }
            if (v < 1.0e19) {
                putFixed(v, negative, sigDigits);
                return;
            }
            int exponent = 0;
            while (v >= 1.0e23) {
                v /= 1.0e22;
                exponent += 22;
            }
            while (v >= 10.0) {
                v /= 10.0;
                exponent++;
            }
            putFixed(v, negative, sigDigits);
            put('e');
            putUnsigned((uint64_t)exponent);
        }

        /** @brief v in [0, 1e19). 32-bit math whenever it fits: 64-bit division is a slow library call on the M0+ */
        void putFixed(double v, bool negative, int sigDigits) {
            const bool small = v < 4294967296.0;
            uint64_t intPart = small ? (uint32_t)v : (uint64_t)v;
            const int intDigits = small ? countDigits((uint32_t)intPart) : countDigits(intPart);
            int decimals = sigDigits - intDigits;
            if (decimals < 0) decimals = 0;
            uint64_t scale = 1;
            for (int i = 0; i < decimals; i++) scale *= 10;
            const bool scale32 = scale <= 1000000000u;
            const double scaledFrac = (v - (double)intPart) * (double)scale + 0.5;
            uint64_t frac = scale32 ? (uint32_t)scaledFrac : (uint64_t)scaledFrac;
            if (frac >= scale) {
                intPart++;
                frac -= scale;
            }
            if (negative && (intPart || frac)) put('-');
            putUnsigned(intPart);
            if (scale32) putDecimals((uint32_t)frac, decimals);
            else putDecimals(frac, decimals);
        }

        template <typename U>
        static int countDigits(U v) {
            int n = 0;
            for (; v; v /= 10) n++;
            return n;
        }

        template <typename U>
        void putDecimals(U frac, int decimals) {
            while (decimals > 0 && frac % 10 == 0) {
                frac /= 10;
                decimals--;
            }
            if (decimals <= 0) return;
            char tmp[20];
            for (int i = decimals - 1; i >= 0; i--) {
                tmp[i] = (char)('0' + frac % 10);
                frac /= 10;
            }
            put('.');
            for (int i = 0; i < decimals; i++) put(tmp[i]);
        }
    };
};

template <const char* Def>
constexpr typename BasicLoggerHelper::HeaderOf<Def>::Storage BasicLoggerHelper::HeaderOf<Def>::value;
template <const char* Def>
constexpr typename BasicLoggerHelper::FieldsOf<Def>::Table BasicLoggerHelper::FieldsOf<Def>::value;

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_BASICLOGGERHELPER_H
