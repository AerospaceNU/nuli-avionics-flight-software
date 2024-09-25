#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H

#include <Avionics.h>
#include <HardwareAbstraction.h>

#define CreateGetter(memberVariableName) decltype(memberVariableName) get##memberVariableName() const { return memberVariableName; }
#define CreateSetter(memberVariableName) void set##memberVariableName(decltype(memberVariableName) newValue) { memberVariableName = newValue; m_flashWriteRequired = true; }
#define CreateGetterSetter(memberVariableName) CreateGetter(memberVariableName) CreateSetter(memberVariableName)

class Configuration {
private:
    // Naming standard is broken to allow for automatic creating on getter/setter methods
    struct {
        double LaunchAltitude = 0;
        double AmbientTemperatureC = 20;
        int32_t RadioChannel = 1;
    } remove_padding;

    bool m_flashWriteRequired = false;

public:
    void setup(HardwareAbstraction* hardware);

    CreateGetterSetter(LaunchAltitude)

    CreateGetterSetter(AmbientTemperatureC)

    CreateGetterSetter(RadioChannel)

    void updateFlash() const {
        if (m_flashWriteRequired) {
//            m_hardware.
        }
    }

private:
    HardwareAbstraction* m_hardware = nullptr;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
