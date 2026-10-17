#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPDEBUG_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPDEBUG_H

#include "Avionics.h"
#include "core/generic_hardware/GenericHardware.h"
#include <cstdio>

class DesktopDebug final : public DebugStream {
public:
    explicit DesktopDebug(FILE* file = stdout) : m_file(file) {
        m_isHighBandwidth = true;
    }

    void setFile(FILE* file) {
        m_file = file;
    }

    size_t write(const void* buffer, const size_t size) override {
        return m_file ? fwrite(buffer, 1, size, m_file) : size;
    }

private:
    FILE* m_file;
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPDEBUG_H
