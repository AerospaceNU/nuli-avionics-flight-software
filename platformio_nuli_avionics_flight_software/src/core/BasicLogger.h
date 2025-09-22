#ifndef BASICLOGGER_H
#define BASICLOGGER_H

#include "Avionics.h"
#include "HardwareAbstraction.h"
#include "Configuration.h"
#include "cli/Parser.h"

template <typename LogDataStruct>
class BasicLogger {
    // clang-format off
    struct InternalStruct_s {
        uint8_t id = 0;
        LogDataStruct data;
    } remove_struct_padding;
    // clang-format on
public:
    void setup(HardwareAbstraction* hardware, Parser* parser, const uint8_t flashID) {
        m_hardware = hardware;
        m_flash = m_hardware->getFlashMemory(flashID);
        m_logWriteIndex = 0;

        for (; true; m_logWriteIndex++) {
            uint8_t id;
            offload(m_logWriteIndex, id);
            if (id == 0xFF) {
                break;
            }
        }

        Serial.print("Starting logging at ");
        Serial.print(getEntryNumber());
        Serial.print(" entry, index: ");
        Serial.println(m_logWriteIndex * sizeof(InternalStruct_s));
    }

    void log(const LogDataStruct& logDataStruct) {
        m_dataStruct.id = 0x01;
        m_dataStruct.data = logDataStruct;
        m_flash->write(m_logWriteIndex * sizeof(InternalStruct_s), m_dataStructStart, sizeof(InternalStruct_s), true);
        m_logWriteIndex++;
    }

    void erase() {
        m_flash->eraseAll();
        m_logWriteIndex = 0;
    }

    LogDataStruct offload(const uint32_t index, uint8_t& id) {
        m_flash->read(index * sizeof(InternalStruct_s), m_dataStructStart, sizeof(InternalStruct_s));
        id = m_dataStruct.id;
        return m_dataStruct.data;
    }

    uint32_t getEntryNumber() const {
        return m_logWriteIndex;
    }

private:
    HardwareAbstraction* m_hardware = nullptr;
    FlashMemory* m_flash = nullptr;
    uint32_t m_logWriteIndex = 0;

    InternalStruct_s m_dataStruct{};
    uint8_t* m_dataStructStart = (uint8_t*)&m_dataStruct;
};

#endif //BASICLOGGER_H
