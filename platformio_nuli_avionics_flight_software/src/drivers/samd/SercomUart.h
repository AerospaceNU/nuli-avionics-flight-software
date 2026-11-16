#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERCOMUART_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERCOMUART_H

#include <Arduino.h>
#include "wiring_private.h"

class SercomUart : public Uart {
public:
    SercomUart(SERCOM* sercom, const uint8_t rxPin, const uint8_t txPin, const SercomRXPad rxPad, const SercomUartTXPad txPad, const EPioType pinMode = PIO_SERCOM_ALT) :
        Uart(sercom, rxPin, txPin, rxPad, txPad), m_rxPin(rxPin), m_txPin(txPin), m_pinMode(pinMode) {}

    void begin(unsigned long baudRate) override {
        Uart::begin(baudRate);
        pinPeripheral(m_rxPin, m_pinMode);
        pinPeripheral(m_txPin, m_pinMode);
    }

    void begin(unsigned long baudRate, uint16_t config) override {
        Uart::begin(baudRate, config);
        pinPeripheral(m_rxPin, m_pinMode);
        pinPeripheral(m_txPin, m_pinMode);
    }

private:
    uint8_t m_rxPin;
    uint8_t m_txPin;
    EPioType m_pinMode;
};

#if defined(__SAMD51__)
#define SERCOM_UART_HANDLERS(sercomIndex, uart)                         \
    extern "C" {                                                        \
    void SERCOM##sercomIndex##_0_Handler() { (uart).IrqHandler(); }     \
    void SERCOM##sercomIndex##_1_Handler() { (uart).IrqHandler(); }     \
    void SERCOM##sercomIndex##_2_Handler() { (uart).IrqHandler(); }     \
    void SERCOM##sercomIndex##_3_Handler() { (uart).IrqHandler(); }     \
    }
#else
#define SERCOM_UART_HANDLERS(sercomIndex, uart)                         \
    extern "C" {                                                        \
    void SERCOM##sercomIndex##_Handler() { (uart).IrqHandler(); }       \
    }
#endif

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SERCOMUART_H
