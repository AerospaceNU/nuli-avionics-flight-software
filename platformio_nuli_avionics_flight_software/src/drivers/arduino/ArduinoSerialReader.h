#ifndef ARDUINOSERIALREADER_H
#define ARDUINOSERIALREADER_H

#include <Arduino.h>
#include "Avionics.h"
#include "core/generic_hardware/LineReader.h"

/**
 * @class ArduinoSerialReader
 * @brief Reads newline-terminated text lines from the Arduino serial interface into a fixed-size buffer.
 * @details  This class provides a simple line reader implementation that reads characters from the global
 * Arduino Serial interface. Characters are stored in an internal buffer up to a fixed capacity N.
 * When a newline character ('\n') is received, the buffer is null-terminated and the function returns true.
 * Optionally, received characters can be echoed back to the serial output as they are read.
 * @tparam N The maximum size of the line buffer, including the null terminator.
 */
template <unsigned N>
class ArduinoSerialReader final : public LineReader {
public:
    /**
     * @brief Constructs a new ArduinoSerialReader.
     * @details  Initializes the internal buffer and configures whether to echo received characters.
     * @param echo If each received character will be echoed back to the serial output.
     */
    explicit ArduinoSerialReader(const bool echo = false) : m_echo(echo) {}

    /**
     * @brief Reads characters from the serial buffer until a full line is received.
     * @details This function reads all available characters from Serial. Characters are appended to an internal
     * buffer until either the buffer limit N - ` is reached or a newline character ('\n') is received.
     * When a newline is encountered, the buffer is null-terminated, the read index is reset,
     * and the function returns `true` to indicate that a full line is ready.
     * If echo mode is enabled, each received character is also immediately sent back via Serial.print()
     * @return if a complete line ending in `'\n'` was read and is ready to be retrieved with getLine()
     */
    bool readLine() override {
        while (Serial.available() > 0) {
            const char c = Serial.read();
            if (m_serialReadIndex < N - 1) {
                m_serialRead[m_serialReadIndex++] = c;
                if (m_echo) {
                    Serial.print(c);
                }
            }
            if (c == '\n') {
                m_serialRead[m_serialReadIndex] = '\0';
                m_serialReadIndex = 0;
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Returns a pointer to the internal line buffer.
     * @details The returned buffer contains the most recently read line, null-terminated.
     * The buffer is overwritten on the next successful call to `readLine()`.
     * @return A pointer to the internal character buffer containing the last line.
     */
    char* getLine() override {
        return m_serialRead;
    }

private:
    bool m_echo = false; ///< Whether to echo received characters back to the serial output.
    char m_serialRead[N] = ""; ///< Internal buffer used to store the current line being read.
    uint32_t m_serialReadIndex = 0; ///< Current write index within the line buffer.
};

#endif //ARDUINOSERIALREADER_H
