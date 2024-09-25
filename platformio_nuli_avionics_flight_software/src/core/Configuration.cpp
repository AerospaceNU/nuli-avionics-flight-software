#include "Configuration.h"
#include "HardwareAbstraction.h"

void Configuration::setup(HardwareAbstraction* hardware) {
    m_hardware = hardware;
}
