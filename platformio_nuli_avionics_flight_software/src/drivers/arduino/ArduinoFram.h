#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOFRAM_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOFRAM_H

#include "Avionics.h"
#include "Arduino.h"
#include "../../core/generic_hardware/ConfigurationMemory.h"
#include "Adafruit_FRAM_SPI.h"

class ArduinoFram : public ConfigurationMemory {
public:
    explicit ArduinoFram(int8_t csPin);

    void setup() override;

    bool ready() override;

    void write(uint32_t address, const uint8_t* buffer, uint32_t length) override;

    void read(uint32_t address, uint8_t* buffer, uint32_t length) override;

private:
    Adafruit_FRAM_SPI m_AdafruitFram;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ARDUINOFRAM_H
