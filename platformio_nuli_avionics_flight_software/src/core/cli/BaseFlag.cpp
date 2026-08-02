#include "BaseFlag.h"

BaseFlag::BaseFlag(const char* name, const char* helpText, bool required, const std::function<void(DebugStream*)> &callback, bool highBandwidthOnly) :
        m_name(name), m_helpText(helpText), m_required(required), m_highBandwidthOnly(highBandwidthOnly), m_set(false), m_callback(callback) {}

void BaseFlag::setDependency(BaseFlag *flag) {
  m_dependency = flag;
}
