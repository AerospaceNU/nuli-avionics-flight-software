#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USB_MSC_OFFLOAD_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USB_MSC_OFFLOAD_H

#include <Avionics.h>
#include <cstring>
#include <cstdlib>
#include <type_traits>
#include "core/generic_hardware/FlashMemory.h"
#include "core/generic_hardware/DebugStream.h"
#include "core/configuration/Configuration.h"
#include "core/configuration/ConfigurationRegistryWraper.h"

// Exposes a virtual FAT16 USB Mass Storage volume that streams flight logs
// from flash as TSV files, one FLIGHTNN.TSV per boot recorded on the chip,
// plus a CONFIG.TXT round-trip wired to the live Configuration object.
//
// Flash format expected (matches BasicLogger): contiguous packed entries of
// `{ uint8_t id; LogDataStruct data; }` starting at offset 0, with
//   id == 0x01 -> data row
//   id == 0x02 -> "new flight" marker (BasicLogger.newFlight())
//   id == 0x03 -> string message (BasicLogger.logMessage())
//   id == 0xFF -> empty / end-of-log
// Flights are divided by the "Logger setup" boot marker BasicLogger writes
// from setup() (a 0x03 entry whose data starts with that literal string).
//
// CONFIG.TXT format is `KEY=value` lines (one per ConfigID passed in the
// template parameter pack). Lines starting with `#` and blank lines are
// ignored. On save (msc_flush_cb), each parsed value is fed to the live
// Configuration object via getConfigurable<>().set(), which goes through
// the registered isValid() check. Invalid values are dropped silently
// (Configuration::set returns false), so the file save can be a no-op for
// out-of-range edits.
//
// TinyUSB callbacks are raw C functions, so the implementation keeps a single
// file-static instance pointer in UsbMscOffload.cpp. Only one usable instance
// per build.

namespace UsbMscOffloadDetail {
    using EntryFormatterFn = void (*)(const uint8_t* entry, DebugStream* debug);
    using RenderConfigFn   = uint32_t (*)(Configuration* cfg, char* buf, uint32_t cap);
    using ParseConfigFn    = void (*)(Configuration* cfg, const char* text, uint32_t len, DebugStream* debug);

    // Local copy of the trait from ConfigurationCliBinding.h so we don't have
    // to pull in the CLI parser headers here.
    template <typename T> struct is_config_string : std::false_type {};
    template <unsigned N> struct is_config_string<ConfigurationString<N>> : std::true_type {};

    // DebugStream that captures formatted output into a caller-supplied buffer.
    // Used to reuse DebugStream's %f-capable formatter (newlib-nano's snprintf
    // silently drops floats on SAMD21) for both log rows and config rendering.
    class BufferDebugStream final : public DebugStream {
    public:
        void bind(char* buf, uint32_t cap) { m_buf = buf; m_cap = cap; m_len = 0; }
        uint32_t length() const { return m_len; }

    protected:
        size_t write(const void* buffer, size_t size) override {
            if (m_buf == nullptr) return 0;
            uint32_t toCopy = (uint32_t)size;
            if (m_len + toCopy > m_cap) toCopy = m_cap - m_len;
            memcpy(m_buf + m_len, buffer, toCopy);
            m_len += toCopy;
            return toCopy;
        }

    private:
        char*    m_buf = nullptr;
        uint32_t m_cap = 0;
        uint32_t m_len = 0;
    };

    void beginInternal(FlashMemory* flash, uint16_t entrySize,
                       const char* tsvHeader, EntryFormatterFn formatter,
                       Configuration* configuration, DebugStream* debug,
                       RenderConfigFn renderFn, ParseConfigFn parseFn);
}

template <typename LogDataStruct, unsigned... ConfigIDs>
class UsbMscOffload {
public:
    // Same signature as a BasicLogger printFunction -- pass `printLog` directly.
    using PrintFn = void (*)(const LogDataStruct&, DebugStream*);

    UsbMscOffload(FlashMemory* flash, const char* tsvHeader, PrintFn printFn,
                  Configuration* configuration, DebugStream* debug = nullptr)
        : m_flash(flash), m_header(tsvHeader),
          m_configuration(configuration), m_debug(debug) {
        s_printFn = printFn;
    }

    // Call once from setup() after the flash chip is initialized AND
    // configuration.setup() has been called.
    void begin() {
        UsbMscOffloadDetail::beginInternal(
            m_flash,
            (uint16_t)(1u + sizeof(LogDataStruct)),
            m_header,
            &entryFormatter,
            m_configuration,
            m_debug,
            &renderAllConfigs,
            &parseAllConfigs);
    }

private:
    FlashMemory*  m_flash;
    const char*   m_header;
    Configuration* m_configuration;
    DebugStream*  m_debug;
    static PrintFn s_printFn;

    // ============================ Log row formatter ============================

    // Glue passed down to the (non-templated) cpp. Branches on the id byte
    // and feeds the user's PrintFn the aligned data payload. Data is copied
    // out byte-for-byte because the raw flash buffer isn't aligned for floats
    // / uint32 on SAMD21.
    static void entryFormatter(const uint8_t* entry, DebugStream* debug) {
        const uint8_t id = entry[0];
        if (id == 0x01) {
            LogDataStruct d;
            memcpy(&d, entry + 1, sizeof(d));
            if (s_printFn != nullptr) s_printFn(d, debug);
        } else if (id == 0x02) {
            debug->data("# === new flight ===");
        } else if (id == 0x03) {
            char str[sizeof(LogDataStruct) + 1];
            memcpy(str, entry + 1, sizeof(LogDataStruct));
            str[sizeof(LogDataStruct)] = '\0';
            debug->data("# %s", str);
        }
    }

    // ============================ Config render ============================

    // Renders all ConfigIDs as `KEY=value` lines into buf. The remaining
    // capacity is padded with CRLF pairs so the file is a clean text view.
    static uint32_t renderAllConfigs(Configuration* cfg, char* buf, uint32_t cap) {
        UsbMscOffloadDetail::BufferDebugStream stream;
        stream.bind(buf, cap);
        stream.data("# NULI USB config -- edit values and save to update FRAM");
        // Pack-expand: emit one line per ConfigID. The leading 0 keeps the
        // initializer non-empty if ConfigIDs... is empty.
        int dummy[] = { 0, (writeOne<ConfigIDs>(cfg, &stream), 0)... };
        (void)dummy;
        uint32_t n = stream.length();
        while (n + 2 <= cap) { buf[n++] = '\r'; buf[n++] = '\n'; }
        return n;
    }

    template <unsigned ID>
    static void writeOne(Configuration* cfg, DebugStream* stream) {
        using T = typename GetConfigurationType_s<ID>::type;
        ConfigurationData<T> data = cfg->getConfigurable<ID>();
        if (!data.isValid()) return;  // ID not in this build's Configuration
        writeKeyValue(stream, GetConfigurationType_s<ID>::name, data.get());
    }

    template <typename T>
    static typename std::enable_if<UsbMscOffloadDetail::is_config_string<T>::value>::type
    writeKeyValue(DebugStream* s, const char* name, const T& v) {
        s->data("%s=%s", name, v.str);
    }

    template <typename T>
    static typename std::enable_if<std::is_floating_point<T>::value>::type
    writeKeyValue(DebugStream* s, const char* name, const T& v) {
        s->data("%s=%.6f", name, (double)v);
    }

    template <typename T>
    static typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type
    writeKeyValue(DebugStream* s, const char* name, const T& v) {
        s->data("%s=%lu", name, (unsigned long)v);
    }

    template <typename T>
    static typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type
    writeKeyValue(DebugStream* s, const char* name, const T& v) {
        s->data("%s=%ld", name, (long)v);
    }

    template <typename T>
    static typename std::enable_if<!std::is_integral<T>::value && !std::is_floating_point<T>::value && !UsbMscOffloadDetail::is_config_string<T>::value>::type
    writeKeyValue(DebugStream* s, const char* name, const T&) {
        s->data("%s=(unsupported type)", name);
    }

    // ============================ Config parse ============================

    static void parseAllConfigs(Configuration* cfg, const char* text, uint32_t len, DebugStream* debug) {
        uint32_t i = 0;
        while (i < len) {
            const uint32_t lineStart = i;
            while (i < len && text[i] != '\n' && text[i] != '\r') i++;
            const uint32_t lineEnd = i;
            while (i < len && (text[i] == '\r' || text[i] == '\n')) i++;

            uint32_t s = lineStart;
            while (s < lineEnd && (text[s] == ' ' || text[s] == '\t')) s++;
            if (s == lineEnd || text[s] == '#') continue;

            uint32_t eq = s;
            while (eq < lineEnd && text[eq] != '=') eq++;
            if (eq == lineEnd) continue;

            char key[48] = {0};
            char val[128] = {0};
            uint32_t kLen = eq - s;
            if (kLen >= sizeof(key)) kLen = sizeof(key) - 1;
            memcpy(key, text + s, kLen);
            uint32_t vLen = lineEnd - eq - 1;
            if (vLen >= sizeof(val)) vLen = sizeof(val) - 1;
            memcpy(val, text + eq + 1, vLen);
            trimEol(key);
            trimEol(val);

            bool matched = false;
            int dummy[] = { 0, (matched = matched || tryParseOne<ConfigIDs>(cfg, key, val, debug), 0)... };
            (void)dummy;
            if (!matched && debug != nullptr) {
                debug->warn("[CONFIG] unknown key: %s", key);
            }
        }
    }

    static void trimEol(char* s) {
        size_t len = strlen(s);
        while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
                           s[len - 1] == '\r' || s[len - 1] == '\n')) {
            s[--len] = '\0';
        }
    }

    template <unsigned ID>
    static bool tryParseOne(Configuration* cfg, const char* key, const char* value, DebugStream* debug) {
        if (strcmp(key, GetConfigurationType_s<ID>::name) != 0) return false;
        using T = typename GetConfigurationType_s<ID>::type;
        ConfigurationData<T> data = cfg->getConfigurable<ID>();
        if (!data.isValid()) return false;
        T parsed{};
        parseValue(value, parsed);
        const bool ok = data.set(parsed);
        if (debug != nullptr) {
            if (ok) debug->message("[CONFIG] set %s = %s", key, value);
            else    debug->warn("[CONFIG] rejected invalid value for %s: %s", key, value);
        }
        return true;
    }

    template <typename T>
    static typename std::enable_if<UsbMscOffloadDetail::is_config_string<T>::value>::type
    parseValue(const char* in, T& out) {
        std::strncpy(out.str, in, sizeof(out.str) - 1);
        out.str[sizeof(out.str) - 1] = '\0';
    }

    template <typename T>
    static typename std::enable_if<std::is_floating_point<T>::value>::type
    parseValue(const char* in, T& out) { out = (T)std::atof(in); }

    template <typename T>
    static typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type
    parseValue(const char* in, T& out) { out = (T)std::strtoul(in, nullptr, 10); }

    template <typename T>
    static typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type
    parseValue(const char* in, T& out) { out = (T)std::strtol(in, nullptr, 10); }

    template <typename T>
    static typename std::enable_if<!std::is_integral<T>::value && !std::is_floating_point<T>::value && !UsbMscOffloadDetail::is_config_string<T>::value>::type
    parseValue(const char*, T&) { /* unsupported -- silently ignored */ }
};

template <typename LogDataStruct, unsigned... ConfigIDs>
typename UsbMscOffload<LogDataStruct, ConfigIDs...>::PrintFn
UsbMscOffload<LogDataStruct, ConfigIDs...>::s_printFn = nullptr;

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USB_MSC_OFFLOAD_H
