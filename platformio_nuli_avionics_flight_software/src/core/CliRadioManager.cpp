//
// Created by chris on 4/17/2025.
//

#include "CliRadioManager.h"

#include "../include/cli/BaseFlag.h"
#include "../include/cli/SimpleFlag.h."
#include "../include/cli/ArgumentFlag.h"
#include "../include/RadioPacketDefinitions.h"

CliRadioManager::CliRadioManager(Parser &parser, RadioLink &radio) : m_parserObject(parser),
                                                                     m_radioObject(radio) { }


void CliRadioManager::startCliLoop(char *input) {
//    m_parserObject.parse(input);
//    m_parserObject.runFlags();
}
