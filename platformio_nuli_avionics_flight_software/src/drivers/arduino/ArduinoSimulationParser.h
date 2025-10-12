#ifndef SIMULATIONPARSER_H
#define SIMULATIONPARSER_H

#include <Arduino.h>

class ArduinoSimulationParser {
public:
    bool blockingGetNextSimulationData() {
        if (m_active) {
            Serial.println("--con");
            uint32_t index = 0;
            m_fieldNumber = 0;
            const uint32_t startTime = millis();
            while (true) {
                if (millis() - startTime > 1000) {
                    m_active = false;
                    return isActive();
                }
                if (Serial.available()) {
                    const char c = Serial.read();
                    if (c == '\n' || index >= sizeof(m_buff) - 1) {
                        m_buff[index] = '\0'; // null terminate
                        break;
                    }
                    m_buff[index++] = c;
                }
            }
        }
        return isActive();
    }

    uint32_t getNextUnsignedInteger() {
        const char* token = getNextToken();
        return token ? strtoul(token, nullptr, 10) : 0;
    }

    float getNextFloat() {
        const char* token = getNextToken();
        return token ? strtof(token, nullptr) : 0.0f;
    }

    void activate() {
        m_active = true;
    }

    void deactivate() {
        m_active = false;
    }

    bool isActive() const {
        return m_active;
    }

private:
    bool m_active = false;
    char m_buff[200] = {};
    uint32_t m_fieldNumber = 0;

    char* getNextToken() {
        char* token = nullptr;
        if (m_fieldNumber == 0) {
            // first call: start at beginning of buff
            token = strtok(m_buff, ",");
        } else {
            // subsequent calls: continue
            token = strtok(nullptr, ",");
        }
        m_fieldNumber++;
        return token;
    }
};

#endif //SIMULATIONPARSER_H
