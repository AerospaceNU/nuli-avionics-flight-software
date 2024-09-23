#include "Configuration.h"
#include "HardwareInterface.h"

void Configuration::setup(HardwareInterface* hardware) {
    m_hardware = hardware;
}
