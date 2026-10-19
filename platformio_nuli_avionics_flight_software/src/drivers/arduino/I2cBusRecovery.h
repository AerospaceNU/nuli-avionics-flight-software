#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_I2CBUSRECOVERY_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_I2CBUSRECOVERY_H

#include <Arduino.h>

struct I2cBusRecoveryResult {
    bool sdaWasLow = false;
    bool sclWasLow = false;
    uint8_t clocks = 0;
    bool released = false;
};

/**
 * @brief Frees an I2C bus a device is holding, with the I2C spec's bus-clear sequence (UM10204 3.1.16)
 * @details A device that was mid-byte when the MCU reset keeps holding SDA until it gets the rest of its clocks.
 * Call before Wire.begin() or after Wire.end(). Takes ~0.2 ms, at most ~1.4 ms.
 */
inline I2cBusRecoveryResult recoverI2cBus(const uint8_t sdaPin, const uint8_t sclPin) {
    constexpr uint32_t HALF_PERIOD_US = 5;
    constexpr uint8_t MAX_CLOCKS = 16;
    constexpr uint32_t SCL_STRETCH_ALLOWANCE_US = 1000;

    I2cBusRecoveryResult result;
    pinMode(sdaPin, INPUT_PULLUP);
    pinMode(sclPin, INPUT_PULLUP);
    delayMicroseconds(HALF_PERIOD_US);

    if (digitalRead(sclPin) == LOW) {
        result.sclWasLow = true;
        const uint32_t start = micros();
        while (digitalRead(sclPin) == LOW && micros() - start < SCL_STRETCH_ALLOWANCE_US) {}
        if (digitalRead(sclPin) == LOW) return result;
    }

    result.sdaWasLow = digitalRead(sdaPin) == LOW;
    while (digitalRead(sdaPin) == LOW && result.clocks < MAX_CLOCKS) {
        digitalWrite(sclPin, LOW);
        pinMode(sclPin, OUTPUT);
        delayMicroseconds(HALF_PERIOD_US);
        pinMode(sclPin, INPUT_PULLUP);
        delayMicroseconds(HALF_PERIOD_US);
        result.clocks++;
    }

    digitalWrite(sdaPin, LOW);
    pinMode(sdaPin, OUTPUT);
    delayMicroseconds(HALF_PERIOD_US);
    pinMode(sdaPin, INPUT_PULLUP);
    delayMicroseconds(HALF_PERIOD_US);

    result.released = digitalRead(sdaPin) == HIGH && digitalRead(sclPin) == HIGH;
    return result;
}

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_I2CBUSRECOVERY_H
