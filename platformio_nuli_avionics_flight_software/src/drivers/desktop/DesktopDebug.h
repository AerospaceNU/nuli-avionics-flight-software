#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H

#include "Avionics.h"
#include "ConstantsUnits.h"
#include "core/generic_hardware/DebugStream.h"
#include <iostream>
#include <fstream>
#include <string>

class DesktopDebug final : public DebugStream {
public:
    DesktopDebug() = default;

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

private:
    std::ofstream fileStream;
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
