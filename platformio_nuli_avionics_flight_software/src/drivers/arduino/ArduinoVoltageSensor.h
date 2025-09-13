#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOVOLTAGESENSOR_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOVOLTAGESENSOR_H

#include "../../core/generic_hardware/VoltageSensor.h"

class ArduinoVoltageSensor final : public VoltageSensor {
public:
    ArduinoVoltageSensor(uint8_t pin, float scaleFactor);

    void read() override;

protected:
    uint8_t m_pin;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOVOLTAGESENSOR_H
