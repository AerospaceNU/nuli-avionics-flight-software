#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H

#include <Avionics.h>
#include <HardwareAbstraction.h>

class Configuration {
public:
    virtual void setup() {

    }

    bool requiresWrite() const {
        return m_updated;
    }

    const void *rawData() {
        return m_buffer;
    }

    const uint32_t length() const {
        return m_length;
    }

protected:
    virtual void setBuffer(void *configuration, uint32_t length) {
        m_buffer = configuration;
        m_length = length;
    }

    inline void raiseUpdate() {
        m_updated = true;
    }

private:
    bool m_updated = false;
    void *m_buffer = nullptr;
    uint32_t m_length = 0;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
