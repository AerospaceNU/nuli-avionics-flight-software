#include "Configuration.h"
#include "HardwareAbstraction.h"

void Configuration::setup(HardwareAbstraction* hardware) {
    m_hardware = hardware;
}

void Configuration::writeFlashIfUpdated() const {
    if (m_flashWriteRequired) {
//            m_hardware.
    }
}
