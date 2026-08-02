#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DEBUGSTREAM_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DEBUGSTREAM_H

#include "Avionics.h"
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>

class DebugStream {
public:
    virtual ~DebugStream() = default;

    // Can be called multiple times, implementations must handle safely
    virtual void setup() {

    }

    // Reads characters until a full line is received; returns true once one is ready via getLine()
    virtual bool readLine() {
        return false;
    }

    // Returns the most recently read line, null-terminated. Overwritten by the next readLine().
    virtual char* getLine() {
        return nullptr;
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

    // Retries on a zero-byte write (brief USB backpressure) instead of dropping data, up to
    // MAX_CONSECUTIVE_STALLS stalls - past that the host is presumed gone, and it returns false.
    static constexpr uint16_t MAX_CONSECUTIVE_STALLS = 10000;

    bool writeRaw(const void* buffer, size_t size) {
        const uint8_t* p = static_cast<const uint8_t*>(buffer);
        uint16_t consecutiveStalls = 0;
        while (size > 0) {
            size_t n = write(p, size);
            // Arduino SAMD's USB CDC returns (uint32_t)-1 on a TX timeout, which a bare
            // "> 0" check would pass through as billions of bytes written - clamp to 0
            // so that hits the stall counter instead of corrupting p/size.
            if (n > size) n = 0;
            if (n == 0) {
                if (++consecutiveStalls >= MAX_CONSECUTIVE_STALLS) return false;
                continue;
            }
            consecutiveStalls = 0;
            p += n;
            size -= n;
        }
        return true;
    }

protected:
    // Override to actually output bytes; returns bytes written. Default is a no-op.
    virtual size_t write(const void* buffer, size_t size) {
        (void)buffer;
        (void)size;
        return 0;
    }

private:
    size_t write(const char* str) {
        return write((const void*)str, strlen(str));
    }

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
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DEBUGSTREAM_H
