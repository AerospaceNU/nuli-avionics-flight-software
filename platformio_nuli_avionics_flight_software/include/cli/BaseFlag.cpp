#include "BaseFlag.h"

BaseFlag::BaseFlag(const char* name, const char* helpText, bool required, uint8_t uid, void (*callback)(uint8_t*, uint32_t length, uint8_t, uint8_t)) :
        m_name(name), m_helpText(helpText), m_required(required), m_identifier(uid), m_set(false), m_callback(callback) {}
