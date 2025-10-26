

#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H

#include "Avionics.h"
#include "ConstantsUnits.h"
#include "core/generic_hardware/DebugStream.h"
#include <iostream>

class DesktopDebug final : public DebugStream {
public:
    size_t write(const void* buffer, const size_t size) override {
        const char* data = static_cast<const char*>(buffer);
        std::cout.write(data, size);
        std::cout.flush(); // ensure immediate output (like Serial)
        return size;
    }
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERIALDEBUG_H
