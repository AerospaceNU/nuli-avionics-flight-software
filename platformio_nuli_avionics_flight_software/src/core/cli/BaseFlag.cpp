#include "BaseFlag.h"

BaseFlag::BaseFlag(const char* name, const char* helpText, bool required, uint8_t uid, const std::function<void(void)> &callback) :
        m_name(name), m_helpText(helpText), m_required(required), m_identifier(uid), m_set(false), m_callback(callback) {}

void BaseFlag::setDependency(BaseFlag *flag) {
  m_dependency = flag;
}
