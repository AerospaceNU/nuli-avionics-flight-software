#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DEBUGSTREAM_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DEBUGSTREAM_H

#include "Avionics.h"
#include "ConstantsUnits.h"

class DebugStream {
public:
    virtual ~DebugStream() = default;

    // ---- Setup ----
    virtual void setup() = 0;

    void setDecimalPlaces(int places) { m_decimalPlaces = places; }

    // ---- Print ----
    virtual void print(const char* str) = 0;
    virtual void print(char c) = 0;
    virtual void print(int32_t num) = 0;
    virtual void print(uint32_t num) = 0;
    virtual void print(double num) = 0;

    // ---- Println ----
    virtual void println() = 0;
    virtual void println(const char* str) = 0;
    virtual void println(char c) = 0;
    virtual void println(int32_t num) = 0;
    virtual void println(uint32_t num) = 0;
    virtual void println(double num) = 0;

    // ---- Write ----
    virtual size_t write(uint8_t b) = 0;
    virtual size_t write(const void* buffer, size_t size) = 0;

protected:
    int m_decimalPlaces = 4;  // default decimal places for floating-point printing
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DEBUGSTREAM_H
