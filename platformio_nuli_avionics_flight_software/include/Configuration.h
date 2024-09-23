#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H

#include <Avionics.h>
#include <HardwareInterface.h>

#define CreateGetter(memberVariableName) decltype(memberVariableName) get##memberVariableName() const { return memberVariableName; }
#define CreateSetter(memberVariableName) void set##memberVariableName(decltype(memberVariableName) newValue) { memberVariableName = newValue; updateFlash(); }
#define CreateGetterSetter(memberVariableName) CreateGetter(memberVariableName) CreateSetter(memberVariableName)

class Configuration {
private:
    struct packed {
        double LaunchAltitude;
        double AmbientTemperature;
        int32_t RadioChannel;
    };

public:
    void setup(HardwareInterface* hardware);

    CreateGetterSetter(LaunchAltitude)

    CreateGetterSetter(AmbientTemperature)

    CreateGetterSetter(RadioChannel)

    void test() {
        double a = getLaunchAltitude();
//        setL
    }

private:
    void updateFlash();

    HardwareInterface* m_hardware;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
