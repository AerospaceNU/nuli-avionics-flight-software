#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H

#include "Avionics.h"
#include "core/generic_hardware/GenericHardware.h"
#include <iostream>
#include <fstream>
#include <string>

#if defined(_WIN32)
#include <conio.h>
#else
#include <unistd.h>
#include <sys/select.h>
#endif

// N is the line buffer capacity (including the null terminator) for readLine()/getLine().
template <unsigned N>
class DesktopDebug final : public DebugStream {
public:
    DesktopDebug() { m_isHighBandwidth = true; } // stdin/stdout, not a slow relayed link

    ~DesktopDebug() override {
        if (fileStream.is_open()) {
            fileStream.close();
        }
    }

    // Optional setup to enable logging to file
    bool outputToFile(const std::string& path) {
        fileStream.open(path, std::ios::out);
        return fileStream.is_open();
    }

    size_t write(const void* buffer, const size_t size) override {
        const char* data = static_cast<const char*>(buffer);

        // Write to console
        std::cout.write(data, size);
        std::cout.flush();

        // Write to file if setup was called
        if (fileStream.is_open()) {
            fileStream.write(data, size);
            fileStream.flush();
        }

        return size;
    }

    // Emulates the Arduino Serial interface for desktop simulations (non-blocking stdin read).
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
                return true;
            }
        }
        return false;
    }

    char* getLine() override {
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

    std::ofstream fileStream;
    char m_serialRead[N] = {};
    uint32_t m_serialReadIndex = 0;
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
