#ifndef SIMULATIONPARSER_H
#define SIMULATIONPARSER_H

#include <Arduino.h>

class SimulationParser {
public:
    void blockingGetNextSimulationData() {
        Serial.println("--con");
        uint32_t index = 0;
        fieldNumber = 0;
        while (true) {
            if (Serial.available()) {
                const char c = Serial.read();
                if (c == '\n' || index >= sizeof(buff) - 1) {
                    buff[index] = '\0';  // null terminate
                    break;
                }
                buff[index++] = c;
            }
        }
    }

    uint32_t getNextUnsignedInteger() {
        const char* token = getNextToken();
        return token ? strtoul(token, nullptr, 10) : 0;
    }

    float getNextFloat() {
        const char* token = getNextToken();
        return token ? strtof(token, nullptr) : 0.0f;
    }

private:
    char buff[200] = {};
    uint32_t fieldNumber = 0;

    char* getNextToken() {
        char* token = nullptr;
        if (fieldNumber == 0) {
            // first call: start at beginning of buff
            token = strtok(buff, ",");
        } else {
            // subsequent calls: continue
            token = strtok(nullptr, ",");
        }
        fieldNumber++;
        return token;
    }
};

#endif //SIMULATIONPARSER_H
