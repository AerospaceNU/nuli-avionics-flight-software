#include "BaseFlag.h"

BaseFlag::BaseFlag(const char* name, const char* helpText, bool required, uint8_t uid, void (*callback)(const char* name, uint8_t*, uint32_t length, uint8_t, uint8_t, BaseFlag*)) :
        m_name(name), m_helpText(helpText), m_required(required), m_identifier(uid), m_set(false), m_callback(callback) {}

void BaseFlag::setDependency(BaseFlag *flag) {
  m_dependency = flag;
}
