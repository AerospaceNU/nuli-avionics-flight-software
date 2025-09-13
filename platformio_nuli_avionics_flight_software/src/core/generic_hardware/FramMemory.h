#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONMEMORY_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONMEMORY_H

class FramMemory {
public:
    virtual void setup() {};

    virtual void write(uint32_t address, const uint8_t* buffer, uint32_t length) = 0;

    virtual void read(uint32_t address, uint8_t* buffer, uint32_t length) = 0;

};

/**
 * @class VolatileConfigurationMemory
 * @brief For board without non-volatile memory for configuration
 * @details Allows the configuration API to be used for boards without
 * non-volatile memory, however it will reset every time the code restarts.
 * This is needed because the configuration API is guaranteed to be available.
 *
 * @todo I think we can remove this
 *
 * @tparam N Size of the memory
 */
template<unsigned N>
class VolatileConfigurationMemory : public FramMemory {
public:
    void write(uint32_t address, const uint8_t* buffer, uint32_t length) override {
        if(length > N) return;
        memccpy(m_buffer, buffer, length);
    }

    void read(uint32_t address, uint8_t* buffer, uint32_t length) override {
        if(length > N) return;
        memccpy(buffer, m_buffer, length);
    }

private:
    uint8_t m_buffer[N]{};
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONMEMORY_H
