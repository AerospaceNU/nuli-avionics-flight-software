#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H

#include <Avionics.h>
#include <HardwareAbstraction.h>

class Configuration {
public:
    void setup(HardwareAbstraction* hardware);

//    template<typename T>
//    uint32_t newConfigurable(const char* name, T* storage) {
//        m_config[m_configureNum].name = name;
//        m_config[m_configureNum].storage = storage;
//        m_config[m_configureNum].size = sizeof(T);
//        m_configureNum++;
//        return m_configureNum - 1;
//    }
//
//    template<typename T>
//    const T* getPtr(uint32_t id) {
//        // Make sure T is the right size
//        if (id >= m_configureNum || sizeof(T) != m_config[id].size) return nullptr;
//        // Set the value
//        return (T*) (m_config[id].storage);
//    }
//
//    template<typename T>
//    T get(uint32_t id) {
//        // Make sure T is the right size
//        if (id >= m_configureNum || sizeof(T) != m_config[id].size) return T();
//        // Set the value
//        return *((T*) (m_config[id].storage));
//    }
//
//
//    template<typename T>
//    void set(uint32_t id, T value) {
//        // Make sure T is the right size
//        if (id >= m_configureNum || sizeof(T) != m_config[id].size) return;
//        // Set the value
//        *((T*) (m_config[id].storage)) = value;
//        m_config[id].updatePending = true;
//    }

    void writeFlashIfUpdated();

private:
    struct ConfigurableInfo_s {
        const char* name = "";
        void* storage = nullptr;
        uint32_t size = 0;
        bool updatePending = false;
    };

    uint32_t m_configureNum = 0;
    ConfigurableInfo_s m_config[20];

    HardwareAbstraction* m_hardware = nullptr;
};

#undef GENERATE_GET_SET_METHODS_MACRO

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
