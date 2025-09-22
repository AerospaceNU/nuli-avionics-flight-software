#ifndef STRINGHELPER_H
#define STRINGHELPER_H

static void reverse_str(char* str, int len) {
    int i = 0, j = len - 1;
    while (i < j) {
        char tmp = str[i];
        str[i++] = str[j];
        str[j--] = tmp;
    }
}

static int int_to_str(int32_t value, char* buf, int base, bool is_unsigned) {
    int i = 0;
    bool neg = false;

    if (!is_unsigned && value < 0) {
        neg = true;
        value = -value;
    }

    uint32_t uvalue = (uint32_t)value;

    do {
        int rem = uvalue % base;
        buf[i++] = (rem < 10) ? ('0' + rem) : ('a' + rem - 10);
        uvalue /= base;
    } while (uvalue != 0);

    if (neg) {
        buf[i++] = '-';
    }

    reverse_str(buf, i);
    buf[i] = '\0';
    return i;
}

static int uint_to_str(uint32_t value, char* buf, int base) {
    return int_to_str((int32_t)value, buf, base, true);
}

static int float_to_str(float f, char* buf, int precision = 2) {
    if (f != f) { // NaN
        buf[0]='N'; buf[1]='a'; buf[2]='N'; buf[3]='\0';
        return 3;
    }

    char* start = buf;

    // Handle negative numbers
    if (f < 0.0f) {
        *buf++ = '-';
        f = -f;
    }

    // Compute rounding factor
    float rounding = 0.5f;
    for (int i = 0; i < precision; ++i)
        rounding /= 10.0f;

    f += rounding; // round to the desired precision

    int int_part = (int)f;
    float frac_part = f - (float)int_part;

    // Convert integer part
    int len = int_to_str(int_part, buf, 10, false);
    buf += len;

    // Add decimal point
    if (precision > 0) {
        *buf++ = '.';
        // Convert fractional part
        for (int i = 0; i < precision; ++i) {
            frac_part *= 10.0f;
            int digit = (int)frac_part;
            *buf++ = '0' + digit;
            frac_part -= digit;
        }
    }

    *buf = '\0';
    return (int)(buf - start);
}

static int mini_snprintf(char* out, int out_size, const char* fmt, ...) {
    char* start = out;
    char* end = out + out_size - 1; // reserve space for null terminator
    va_list args;
    va_start(args, fmt);

    while (*fmt && out < end) {
        if (*fmt == '%') {
            fmt++;
            int precision = 6; // default float precision

            if (*fmt == 'l') { fmt++; }

            // parse optional precision for float
            if (*fmt == '.') {
                fmt++;
                precision = 0;
                while (*fmt >= '0' && *fmt <= '9') {
                    precision = precision * 10 + (*fmt - '0');
                    fmt++;
                }
            }

            char buf[32];
            int len = 0;

            switch (*fmt) {
                case 'd': len = int_to_str(va_arg(args, int), buf, 10, false); break;
                case 'u': len = uint_to_str(va_arg(args, unsigned int), buf, 10); break;
                case 'x': len = uint_to_str(va_arg(args, unsigned int), buf, 16); break;
                case 'f': len = float_to_str((float)va_arg(args, double), buf, precision); break;
                case 'c': buf[0] = (char)va_arg(args, int); buf[1] = '\0'; len = 1; break;
                case 's': {
                    const char* s = va_arg(args, const char*);
                    for (len = 0; s[len] && out + len < end; ++len)
                        buf[len] = s[len];
                    buf[len] = '\0';
                    break;
                }
                case '%': buf[0] = '%'; buf[1] = '\0'; len = 1; break;
                default: buf[0] = '?'; buf[1] = '\0'; len = 1; break;
            }

            for (int i = 0; i < len && out < end; ++i) {
                *out++ = buf[i];
            }
        } else {
            *out++ = *fmt;
        }
        fmt++;
    }

    *out = '\0';
    va_end(args);
    return (int)(out - start);
}


#endif //STRINGHELPER_H
