#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DEBUGSTREAM_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DEBUGSTREAM_H

#include "Avionics.h"
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <cmath>
#include <type_traits>

class DebugStream {
public:
    virtual ~DebugStream() = default;

    virtual void setup() {

    }

    void message(const char* fmt, ...) {
        write("MSG:\t");
        va_list args;
        va_start(args, fmt);
        vformat(fmt, args);
        va_end(args);
        write("\n");
    }

    void warn(const char* fmt, ...) {
        write("WARN:\t");
        va_list args;
        va_start(args, fmt);
        vformat(fmt, args);
        va_end(args);
        write("\n");
    }

    void error(const char* fmt, ...) {
        write("ERROR:\t");
        va_list args;
        va_start(args, fmt);
        vformat(fmt, args);
        va_end(args);
        write("\n");
    }

    void debug(const char* fmt, ...) {
        write("DEBUG:\t");
        va_list args;
        va_start(args, fmt);
        vformat(fmt, args);
        va_end(args);
        write("\n");
    }


    void data(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vformat(fmt, args);
        va_end(args);
        write("\n");
    }

protected:
    // Implement this in your derived class to actually output bytes.
    // Should return number of bytes written. The default is a no-op.
    virtual size_t write(const void* buffer, size_t size) {
        (void)buffer;
        (void)size;
        return 0;
    }

private:
    // small helper to write C string
    size_t write(const char* str) {
        return write((const void*)str, strlen(str));
    }

    // maximum single-format output buffer. Adjust if you need bigger
    static constexpr size_t BUF_SIZE = 512;

    void vformat(const char* fmt, va_list args) {
        char buf[1024]; // bigger buffer for long lines
        size_t bi = 0;

        auto putch = [&](char c) {
            if (bi < sizeof(buf)) buf[bi++] = c;
        };

        auto puts_lit = [&](const char* s) {
            while (*s && bi < sizeof(buf)) buf[bi++] = *s++;
        };

        auto uint_to_str = [](unsigned long long val, unsigned base, bool upper, char* out, size_t outlen) -> size_t {
            const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
            char tmp[32];
            size_t ti = 0;
            if (val == 0) tmp[ti++] = '0';
            else
                while (val && ti < sizeof(tmp)) {
                    tmp[ti++] = digits[val % base];
                    val /= base;
                }
            size_t oi = 0;
            while (ti > 0 && oi + 1 < outlen) out[oi++] = tmp[--ti];
            out[oi] = '\0';
            return oi;
        };

        auto format_integer = [&](unsigned long long uv, bool is_signed, bool negative,
                                  int base, bool upper, int width, char pad) {
            char numbuf[32];
            size_t len = uint_to_str(uv, base, upper, numbuf, sizeof(numbuf));
            if (is_signed && negative) putch('-');
            for (int i = (int)len; i < width; ++i) putch(pad);
            for (size_t i = 0; i < len; ++i) putch(numbuf[i]);
        };

        auto format_float = [&](double val, int precision) {
            if (val < 0) {
                putch('-');
                val = -val;
            }
            unsigned long long int_part = (unsigned long long)val;
            double frac = val - (double)int_part;

            char intbuf[32];
            size_t intlen = uint_to_str(int_part, 10, false, intbuf, sizeof(intbuf));
            for (size_t i = 0; i < intlen; ++i) putch(intbuf[i]);

            if (precision > 0) {
                putch('.');
                for (int i = 0; i < precision; ++i) {
                    frac *= 10.0;
                    int digit = (int)frac;
                    putch('0' + digit);
                    frac -= digit;
                }
            }
        };

        while (*fmt && bi < sizeof(buf)) {
            if (*fmt != '%') {
                putch(*fmt++);
                continue;
            }
            ++fmt;

            // flags (only '0' supported)
            char pad = ' ';
            if (*fmt == '0') {
                pad = '0';
                ++fmt;
            }

            // width
            int width = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                ++fmt;
            }

            // precision
            int precision = -1;
            if (*fmt == '.') {
                ++fmt;
                precision = 0;
                while (*fmt >= '0' && *fmt <= '9') {
                    precision = precision * 10 + (*fmt - '0');
                    ++fmt;
                }
            }

            // length modifiers
            bool long_flag = false;
            bool long_long_flag = false;
            if (*fmt == 'l') {
                ++fmt;
                if (*fmt == 'l') {
                    long_long_flag = true;
                    ++fmt;
                } else {
                    long_flag = true;
                }
            }

            char spec = *fmt++;

            switch (spec) {
            case 'd': {
                long long v;
                if (long_long_flag)
                    v = va_arg(args, long long);
                else if (long_flag)
                    v = va_arg(args, long);
                else
                    v = va_arg(args, int);

                bool neg = v < 0;
                unsigned long long uv = neg ? (unsigned long long)(-v) : (unsigned long long)v;
                format_integer(uv, true, neg, 10, false, width, pad);
                break;
            }

            case 'u': {
                unsigned long long v;
                if (long_long_flag)
                    v = va_arg(args, unsigned long long);
                else if (long_flag)
                    v = va_arg(args, unsigned long);
                else
                    v = va_arg(args, unsigned int);

                format_integer(v, false, false, 10, false, width, pad);
                break;
            }

            case 'x':
            case 'X': {
                unsigned long long v;
                if (long_long_flag)
                    v = va_arg(args, unsigned long long);
                else if (long_flag)
                    v = va_arg(args, unsigned long);
                else
                    v = va_arg(args, unsigned int);

                format_integer(v, false, false, 16, spec == 'X', width, pad);
                break;
            }

            case 'c': {
                putch((char)va_arg(args, int));
                break;
            }

            case 's': {
                const char* s = va_arg(args, const char*);
                if (!s) s = "(null)";
                puts_lit(s);
                break;
            }

            case 'f': {
                double fv = va_arg(args, double);
                if (precision < 0) precision = 6; // default
                if (precision > 9) precision = 9; // cap
                format_float(fv, precision);
                break;
            }

            case 'p': {
                void* ptr = va_arg(args, void*);
                puts_lit("0x");
                unsigned long long v = (uintptr_t)ptr;
                char tmp[32];
                size_t len = uint_to_str(v, 16, false, tmp, sizeof(tmp));
                for (size_t i = 0; i < len; ++i) putch(tmp[i]);
                break;
            }

            case '%':
                putch('%');
                break;

            default:
                putch('%');
                putch(spec);
                break;
            }
        }

        write(buf, bi);
    }

    // void vformat(const char* fmt, va_list args) {
    //     char buf[1024]; // bigger buffer for long lines
    //     size_t bi = 0;
    //
    //     auto putch = [&](char c) {
    //         if (bi < sizeof(buf)) buf[bi++] = c;
    //     };
    //
    //     auto puts_lit = [&](const char* s) {
    //         while (*s && bi < sizeof(buf)) buf[bi++] = *s++;
    //     };
    //
    //     auto uint_to_str = [](unsigned long long val, unsigned base, bool upper, char* out, size_t outlen) -> size_t {
    //         const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    //         char tmp[32];
    //         size_t ti = 0;
    //         if (val == 0) tmp[ti++] = '0';
    //         else while (val && ti < sizeof(tmp)) {
    //             tmp[ti++] = digits[val % base];
    //             val /= base;
    //         }
    //         size_t oi = 0;
    //         while (ti > 0 && oi + 1 < outlen) out[oi++] = tmp[--ti];
    //         out[oi] = '\0';
    //         return oi;
    //     };
    //
    //     auto format_integer = [&](unsigned long long uv, bool is_signed, bool negative,
    //                               int base, bool upper, int width, char pad) {
    //         char numbuf[32];
    //         size_t len = uint_to_str(uv, base, upper, numbuf, sizeof(numbuf));
    //         if (is_signed && negative) putch('-');
    //
    //         for (int i = (int)len; i < width; ++i) putch(pad);
    //         for (size_t i = 0; i < len; ++i) putch(numbuf[i]);
    //     };
    //
    //     auto format_float = [&](double val, int precision) {
    //         if (val < 0) {
    //             putch('-');
    //             val = -val;
    //         }
    //         unsigned long long int_part = (unsigned long long)val;
    //         double frac = val - (double)int_part;
    //
    //         char intbuf[32];
    //         size_t intlen = uint_to_str(int_part, 10, false, intbuf, sizeof(intbuf));
    //         for (size_t i = 0; i < intlen; ++i) putch(intbuf[i]);
    //
    //         if (precision > 0) {
    //             putch('.');
    //             for (int i = 0; i < precision; ++i) {
    //                 frac *= 10.0;
    //                 int digit = (int)frac;
    //                 putch('0' + digit);
    //                 frac -= digit;
    //             }
    //         }
    //     };
    //
    //     while (*fmt && bi < sizeof(buf)) {
    //         if (*fmt != '%') {
    //             putch(*fmt++);
    //             continue;
    //         }
    //         ++fmt;
    //
    //         // flags (only '0' supported)
    //         char pad = ' ';
    //         if (*fmt == '0') {
    //             pad = '0';
    //             ++fmt;
    //         }
    //
    //         // width
    //         int width = 0;
    //         while (*fmt >= '0' && *fmt <= '9') {
    //             width = width * 10 + (*fmt - '0');
    //             ++fmt;
    //         }
    //
    //         // precision
    //         int precision = -1;
    //         if (*fmt == '.') {
    //             ++fmt;
    //             precision = 0;
    //             while (*fmt >= '0' && *fmt <= '9') {
    //                 precision = precision * 10 + (*fmt - '0');
    //                 ++fmt;
    //             }
    //         }
    //
    //         // length modifiers
    //         bool long_flag = false;
    //         if (*fmt == 'l') {
    //             long_flag = true;
    //             ++fmt;
    //         }
    //
    //         char spec = *fmt++;
    //
    //         switch (spec) {
    //         case 'd': {
    //             long v = long_flag ? va_arg(args, long) : va_arg(args, int);
    //             bool neg = v < 0;
    //             unsigned long uv = neg ? -v : v;
    //             format_integer(uv, true, neg, 10, false, width, pad);
    //             break;
    //         }
    //         case 'u':
    //         case 'l':
    //         case 'U':
    //         case 'L': {
    //             unsigned long v = va_arg(args, unsigned long);
    //             format_integer(v, false, false, 10, false, width, pad);
    //             break;
    //         }
    //         case 'x':
    //         case 'X': {
    //             unsigned int v = va_arg(args, unsigned int);
    //             format_integer(v, false, false, 16, spec == 'X', width, pad);
    //             break;
    //         }
    //         case 'c': {
    //             putch((char)va_arg(args, int));
    //             break;
    //         }
    //         case 's': {
    //             const char* s = va_arg(args, const char*);
    //             if (!s) s = "(null)";
    //             puts_lit(s);
    //             break;
    //         }
    //         case 'f': {
    //             double fv = va_arg(args, double);
    //             if (precision < 0) precision = 6; // default
    //             if (precision > 9) precision = 9; // cap
    //             format_float(fv, precision);
    //             break;
    //         }
    //         case 'p': {
    //             void* ptr = va_arg(args, void*);
    //             puts_lit("0x");
    //             unsigned long long v = (uintptr_t)ptr;
    //             char tmp[32];
    //             size_t len = uint_to_str(v, 16, false, tmp, sizeof(tmp));
    //             for (size_t i = 0; i < len; ++i) putch(tmp[i]);
    //             break;
    //         }
    //         case '%': putch('%');
    //             break;
    //         default: putch('%');
    //             putch(spec);
    //             break;
    //         }
    //     }
    //
    //     write(buf, bi);
    // }

    // Primary vformat implementation
    // void vformat(const char* fmt, va_list args) {
    //     char buf[BUF_SIZE];
    //     size_t bi = 0;
    //
    //     auto flush = [&](void) {
    //         if (bi > 0) {
    //             write(buf, bi);
    //             bi = 0;
    //         }
    //     };
    //
    //     auto putch = [&](char c) {
    //         if (bi + 1 >= BUF_SIZE) flush();
    //         buf[bi++] = c;
    //     };
    //
    //     auto puts_lit = [&](const char* s) {
    //         while (*s) {
    //             if (bi + 1 >= BUF_SIZE) flush();
    //             buf[bi++] = *s++;
    //         }
    //     };
    //
    //     // integer -> string helper
    //     auto uint_to_str = [&](unsigned long long val, unsigned base, bool upper,
    //                            char *out, size_t outlen) -> size_t {
    //         const char* digits_l = "0123456789abcdef";
    //         const char* digits_u = "0123456789ABCDEF";
    //         const char* digits = upper ? digits_u : digits_l;
    //
    //         // generate in reverse
    //         char tmp[32];
    //         size_t ti = 0;
    //         if (val == 0) {
    //             tmp[ti++] = '0';
    //         } else {
    //             while (val && ti < sizeof(tmp)) {
    //                 tmp[ti++] = digits[val % base];
    //                 val /= base;
    //             }
    //         }
    //         // reverse into out
    //         size_t oi = 0;
    //         while (ti > 0 && oi + 1 < outlen) {
    //             out[oi++] = tmp[--ti];
    //         }
    //         out[oi] = '\0';
    //         return oi;
    //     };
    //
    //     // signed integer to string with width/pad
    //     auto format_integer = [&](long long sval, unsigned long long uval,
    //                               bool is_signed,
    //                               bool negative,
    //                               unsigned base, bool upper,
    //                               int width, int precision,
    //                               bool left_adj, char pad,
    //                               bool show_sign, bool space_sign, bool alt) {
    //         char numbuf[64];
    //         size_t nlen = 0;
    //
    //         if (is_signed) {
    //             nlen = uint_to_str(static_cast<unsigned long long>(uval), base, upper, numbuf, sizeof(numbuf));
    //         } else {
    //             nlen = uint_to_str(uval, base, upper, numbuf, sizeof(numbuf));
    //         }
    //
    //         // handle precision: minimum digits
    //         size_t prec_pad = 0;
    //         if (precision > 0 && (size_t)precision > nlen) {
    //             prec_pad = precision - (int)nlen;
    //         } else if (precision == 0 && nlen == 1 && numbuf[0] == '0') {
    //             // precision zero and value zero => empty output (printf behaviour)
    //             nlen = 0;
    //             numbuf[0] = '\0';
    //         }
    //
    //         // prefix handling
    //         char prefix[3] = {0};
    //         size_t prefix_len = 0;
    //         if (negative) {
    //             prefix[prefix_len++] = '-';
    //         } else if (show_sign) {
    //             prefix[prefix_len++] = '+';
    //         } else if (space_sign) {
    //             prefix[prefix_len++] = ' ';
    //         }
    //
    //         if (alt) {
    //             if (base == 16 && nlen > 0) {
    //                 prefix[prefix_len++] = '0';
    //                 prefix[prefix_len++] = upper ? 'X' : 'x';
    //             } else if (base == 8 && (nlen == 0 || numbuf[0] != '0')) {
    //                 // ensure leading 0 for octal if alt
    //                 // we'll handle by reducing precision so a leading 0 is printed
    //                 if (prec_pad == 0) prec_pad = 1;
    //             }
    //         }
    //
    //         int total_len = (int)prefix_len + (int)prec_pad + (int)nlen;
    //         int pad_len = 0;
    //         if (width > total_len) pad_len = width - total_len;
    //
    //         if (!left_adj) {
    //             // padding before prefix (unless pad is '0', then after prefix)
    //             if (pad == '0') {
    //                 // prefix first
    //                 for (size_t i = 0; i < prefix_len; ++i) putch(prefix[i]);
    //                 // zero pad
    //                 for (int i = 0; i < pad_len; ++i) putch('0');
    //             } else {
    //                 for (int i = 0; i < pad_len; ++i) putch(' ');
    //                 for (size_t i = 0; i < prefix_len; ++i) putch(prefix[i]);
    //             }
    //         } else {
    //             // left adjust: prefix first
    //             for (size_t i = 0; i < prefix_len; ++i) putch(prefix[i]);
    //         }
    //
    //         // precision zeros
    //         for (size_t i = 0; i < prec_pad; ++i) putch('0');
    //
    //         // digits
    //         for (size_t i = 0; i < nlen; ++i) putch(numbuf[i]);
    //
    //         if (left_adj) {
    //             for (int i = 0; i < pad_len; ++i) putch(' ');
    //         }
    //     };
    //
    //     // floating point formatting: limited, small-footprint implementation
    //     // supports fixed notation %f with precision up to, say, 9 safely
    //     auto format_float = [&](long double val, int width, int precision,
    //                             bool left_adj, char pad, bool show_sign, bool space_sign) {
    //         if (std::isnan((double)val)) {
    //             const char* s = "nan";
    //             if (show_sign) putch('+');
    //             else if (space_sign) putch(' ');
    //             puts_lit(s);
    //             return;
    //         }
    //         if (std::isinf((double)val)) {
    //             if (val < 0) putch('-');
    //             else if (show_sign) putch('+');
    //             else if (space_sign) putch(' ');
    //             const char* s = "inf";
    //             puts_lit(s);
    //             return;
    //         }
    //
    //         bool negative = (val < 0.0L);
    //         if (negative) val = -val;
    //
    //         if (precision < 0) precision = 6; // default
    //         if (precision > 9) precision = 9; // cap to reasonable amount to avoid huge loops
    //
    //         // split integer and fractional
    //         unsigned long long int_part = (unsigned long long)val;
    //         long double frac = val - (long double)int_part;
    //
    //         // fractional rounding
    //         long double mult = 1.0L;
    //         for (int i = 0; i < precision; ++i) mult *= 10.0L;
    //         long double frac_scaled = frac * mult + 0.5L; // round
    //         unsigned long long frac_int = (unsigned long long)(frac_scaled);
    //
    //         // handle carry from rounding (e.g., .999 -> carry)
    //         if (frac_int >= (unsigned long long)mult) {
    //             frac_int = 0;
    //             ++int_part;
    //         }
    //
    //         // convert int_part
    //         char intbuf[32];
    //         size_t intlen = 0;
    //         {
    //             // reuse uint_to_str
    //             intlen = uint_to_str(int_part, 10, false, intbuf, sizeof(intbuf));
    //         }
    //
    //         // fractional digits
    //         char fracbuf[16];
    //         size_t fraclen = 0;
    //         if (precision > 0) {
    //             // produce digits with leading zeros
    //             // write into buffer in reverse then reverse
    //             char tmp[16];
    //             size_t ti = 0;
    //             if (frac_int == 0) {
    //                 for (int i = 0; i < precision; ++i) tmp[ti++] = '0';
    //             } else {
    //                 unsigned long long t = frac_int;
    //                 while (t && ti < (size_t)precision) {
    //                     tmp[ti++] = '0' + (t % 10ULL);
    //                     t /= 10ULL;
    //                 }
    //                 while ((int)ti < precision) tmp[ti++] = '0';
    //             }
    //             // reverse into fracbuf
    //             for (size_t i = 0; i < (size_t)precision && i < sizeof(fracbuf)-1; ++i) {
    //                 fracbuf[i] = tmp[precision - 1 - i];
    //             }
    //             fraclen = precision;
    //             fracbuf[fraclen] = '\0';
    //         } else {
    //             fraclen = 0;
    //             fracbuf[0] = '\0';
    //         }
    //
    //         // compute total length to handle width/padding
    //         int totlen = 0;
    //         if (negative || show_sign || space_sign) totlen += 1;
    //         totlen += (int)intlen;
    //         if (precision > 0) totlen += 1 + (int)fraclen; // '.' + fractional
    //
    //         int pad_len = 0;
    //         if (width > totlen) pad_len = width - totlen;
    //
    //         if (!left_adj) {
    //             if (pad == '0') {
    //                 if (negative) putch('-');
    //                 else if (show_sign) putch('+');
    //                 else if (space_sign) putch(' ');
    //                 for (int i = 0; i < pad_len; ++i) putch('0');
    //             } else {
    //                 for (int i = 0; i < pad_len; ++i) putch(' ');
    //                 if (negative) putch('-');
    //                 else if (show_sign) putch('+');
    //                 else if (space_sign) putch(' ');
    //             }
    //         } else {
    //             if (negative) putch('-');
    //             else if (show_sign) putch('+');
    //             else if (space_sign) putch(' ');
    //         }
    //
    //         // integer part
    //         for (size_t i = 0; i < intlen; ++i) putch(intbuf[i]);
    //
    //         // fractional
    //         if (precision > 0) {
    //             putch('.');
    //             for (size_t i = 0; i < fraclen; ++i) putch(fracbuf[i]);
    //         }
    //
    //         if (left_adj) {
    //             for (int i = 0; i < pad_len; ++i) putch(' ');
    //         }
    //     };
    //
    //     // MAIN parse loop
    //     const char* p = fmt;
    //     while (*p) {
    //         if (*p != '%') {
    //             putch(*p++);
    //             continue;
    //         }
    //         ++p; // skip '%'
    //         if (*p == '%') {
    //             putch('%');
    //             ++p;
    //             continue;
    //         }
    //
    //         // parse flags
    //         bool left_adj = false;
    //         bool show_sign = false;
    //         bool space_sign = false;
    //         bool alt = false;
    //         char pad = ' ';
    //
    //         bool flags_parsed = true;
    //         while (flags_parsed) {
    //             switch (*p) {
    //                 case '-': left_adj = true; ++p; break;
    //                 case '+': show_sign = true; ++p; break;
    //                 case ' ': space_sign = true; ++p; break;
    //                 case '#': alt = true; ++p; break;
    //                 case '0': if (!left_adj) pad = '0'; ++p; break;
    //                 default: flags_parsed = false; break;
    //             }
    //         }
    //
    //         // width
    //         int width = 0;
    //         if (*p == '*') {
    //             ++p;
    //             width = va_arg(args, int);
    //             if (width < 0) { left_adj = true; width = -width; }
    //         } else {
    //             while (*p >= '0' && *p <= '9') {
    //                 width = width * 10 + (*p - '0');
    //                 ++p;
    //             }
    //         }
    //
    //         // precision
    //         int precision = -1;
    //         if (*p == '.') {
    //             ++p;
    //             precision = 0;
    //             if (*p == '*') {
    //                 ++p;
    //                 precision = va_arg(args, int);
    //                 if (precision < 0) precision = -1; // negative precision treated as unspecified
    //             } else {
    //                 while (*p >= '0' && *p <= '9') {
    //                     precision = precision * 10 + (*p - '0');
    //                     ++p;
    //                 }
    //             }
    //         }
    //
    //         // length modifier
    //         int length = 0; // 0=default, 1='l', 2='ll', -1='h', -2='hh' (we only use l/ll here)
    //         if (*p == 'h') {
    //             ++p;
    //             if (*p == 'h') { ++p; length = -2; } else length = -1;
    //         } else if (*p == 'l') {
    //             ++p;
    //             if (*p == 'l') { ++p; length = 2; } else length = 1;
    //         } else if (*p == 'z') { ++p; /*size_t - treat as default*/ }
    //         // conversion specifier
    //         char spec = *p ? *p++ : '\0';
    //
    //         switch (spec) {
    //             case 'd':
    //             case 'i': {
    //                 // signed decimal
    //                 long long v = 0;
    //                 if (length == 2) v = va_arg(args, long long);
    //                 else if (length == 1) v = va_arg(args, long);
    //                 else v = va_arg(args, int);
    //                 bool neg = v < 0;
    //                 unsigned long long uv = neg ? (unsigned long long)(-v) : (unsigned long long)v;
    //                 format_integer(v, uv, true, neg, 10, false, width, precision, left_adj, pad, show_sign, space_sign, false);
    //                 break;
    //             }
    //             case 'u': {
    //                 unsigned long long v = 0;
    //                 if (length == 2) v = va_arg(args, unsigned long long);
    //                 else if (length == 1) v = va_arg(args, unsigned long);
    //                 else v = va_arg(args, unsigned int);
    //                 format_integer(0, v, false, false, 10, false, width, precision, left_adj, pad, false, false, false);
    //                 break;
    //             }
    //             case 'o': {
    //                 unsigned long long v = 0;
    //                 if (length == 2) v = va_arg(args, unsigned long long);
    //                 else if (length == 1) v = va_arg(args, unsigned long);
    //                 else v = va_arg(args, unsigned int);
    //                 format_integer(0, v, false, false, 8, false, width, precision, left_adj, pad, false, false, alt);
    //                 break;
    //             }
    //             case 'x':
    //             case 'X': {
    //                 bool upper = (spec == 'X');
    //                 unsigned long long v = 0;
    //                 if (length == 2) v = va_arg(args, unsigned long long);
    //                 else if (length == 1) v = va_arg(args, unsigned long);
    //                 else v = va_arg(args, unsigned int);
    //                 format_integer(0, v, false, false, 16, upper, width, precision, left_adj, pad, false, false, alt);
    //                 break;
    //             }
    //             case 'c': {
    //                 int ch = va_arg(args, int);
    //                 char c = (char)ch;
    //                 if (!left_adj) {
    //                     for (int i = 1; i < width; ++i) putch(pad);
    //                     putch(c);
    //                 } else {
    //                     putch(c);
    //                     for (int i = 1; i < width; ++i) putch(' ');
    //                 }
    //                 break;
    //             }
    //             case 's': {
    //                 const char* s = va_arg(args, const char*);
    //                 if (!s) s = "(null)";
    //                 size_t slen = strlen(s);
    //                 if (precision >= 0 && (size_t)precision < slen) slen = precision;
    //                 if (!left_adj) {
    //                     for (int i = (int)slen; i < width; ++i) putch(pad);
    //                     for (size_t i = 0; i < slen; ++i) putch(s[i]);
    //                 } else {
    //                     for (size_t i = 0; i < slen; ++i) putch(s[i]);
    //                     for (int i = (int)slen; i < width; ++i) putch(' ');
    //                 }
    //                 break;
    //             }
    //             case 'p': {
    //                 void* ptr = va_arg(args, void*);
    //                 unsigned long long v = (uintptr_t)ptr;
    //                 // platform pointer width handling: print as hex with 0x
    //                 char tmp[32];
    //                 size_t len = uint_to_str(v, 16, false, tmp, sizeof(tmp));
    //                 puts_lit("0x");
    //                 for (size_t i = 0; i < len; ++i) putch(tmp[i]);
    //                 break;
    //             }
    //             case 'f': {
    //                 long double fv;
    //                 // float promotions: float->double in varargs
    //                 if (length == 1 || length == 2) {
    //                     // length 'l' / 'll' for floats isn't meaningful; treat as double
    //                     fv = va_arg(args, double);
    //                 } else {
    //                     fv = va_arg(args, double);
    //                 }
    //                 format_float(fv, width, precision, left_adj, pad, show_sign, space_sign);
    //                 break;
    //             }
    //             case '%': {
    //                 putch('%');
    //                 break;
    //             }
    //             default: {
    //                 // unknown specifier: output it verbatim (like printf)
    //                 putch('%');
    //                 if (spec) putch(spec);
    //                 break;
    //             }
    //         }
    //     } // end while
    //
    //     // flush remaining buffer
    //     if (bi > 0) write(buf, bi);
    // }
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DEBUGSTREAM_H
