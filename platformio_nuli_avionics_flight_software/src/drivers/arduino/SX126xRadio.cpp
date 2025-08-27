/*
 * @TODO Naming
 */

#include "SX126xRadio.h"
#include "RadioPacketDefinitions.h"

volatile bool operationDone = false;

void setFlagGlobal(void) {
    operationDone = true;
}

void SX126xRadio::setup() {
}
