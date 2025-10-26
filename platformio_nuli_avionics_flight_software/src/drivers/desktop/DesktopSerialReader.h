#ifndef DESKTOPSERIALREADER_H
#define DESKTOPSERIALREADER_H

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include "Avionics.h"
#include "core/generic_hardware/LineReader.h"

#if defined(_WIN32)
#include <conio.h>
#else
#include <unistd.h>
#include <sys/select.h>
#endif

/**
 * @class DesktopSerialReader
 * @brief Reads newline-terminated text lines from stdin (non-blocking).
 * @details Emulates the Arduino Serial interface for desktop simulations.
 */
template <unsigned N>
class DesktopSerialReader final : public LineReader {
public:
    DesktopSerialReader() = default;

    bool readLine() override {
        while (inputAvailable()) {
            int c = std::cin.get();
            if (c == EOF)
                break;

            // Normalize Windows-style CRLF
            if (c == '\r')
                continue;

            if (m_serialReadIndex < N - 1) {
                m_serialRead[m_serialReadIndex++] = static_cast<char>(c);
            }

            if (c == '\n') {
                m_serialRead[m_serialReadIndex - 1] = '\0'; // Remove newline
                m_serialReadIndex = 0;
                m_lineReady = true;
                return true;
            }
        }
        return false;
    }

    char* getLine() override {
        m_lineReady = false;
        return m_serialRead;
    }

private:
    bool inputAvailable() {
#if defined(_WIN32)
        return _kbhit();
#else
        fd_set set;
        struct timeval timeout = {0, 0};
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        return select(STDIN_FILENO + 1, &set, nullptr, nullptr, &timeout) > 0;
#endif
    }

    bool m_lineReady = false;
    char m_serialRead[N] = {};
    uint32_t m_serialReadIndex = 0;
};

#endif // DESKTOPSERIALREADER_H
