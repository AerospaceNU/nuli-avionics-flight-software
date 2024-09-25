#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H

#include <Avionics.h>
#include <HardwareAbstraction.h>

/**
 * This macro generates getter and setter for member variables
 *
 * ## works as string concatenation in macros, allowing for the type name to be injected into the method names
 */
#define GENERATE_GET_SET_METHODS_MACRO(Type, Name, memberVariable)      \
inline void set##Name(Type value) {                                     \
    (memberVariable) = value;                                           \
    m_flashWriteRequired = true;                                        \
}                                                                       \
inline Type get##Name() {                                               \
    return memberVariable;                                              \
}

class Configuration {
public:
    void setup(HardwareAbstraction* hardware);

    GENERATE_GET_SET_METHODS_MACRO(double, LaunchAltitude, m_launchAltitude)

    GENERATE_GET_SET_METHODS_MACRO(double, AmbientTemperatureK, m_ambientTemperatureK)

    GENERATE_GET_SET_METHODS_MACRO(double, RadioChannel, m_radioChannel)

    void writeFlashIfUpdated() const;

private:
    struct {
        double m_launchAltitude = 0;
        double m_ambientTemperatureK = 20;
        int32_t m_radioChannel = 1;
    } remove_padding;

    bool m_flashWriteRequired = false;
    HardwareAbstraction* m_hardware = nullptr;
};

#undef GENERATE_GET_SET_METHODS_MACRO

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
