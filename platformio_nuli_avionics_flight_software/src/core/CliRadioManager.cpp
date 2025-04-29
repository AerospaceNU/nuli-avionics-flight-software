//
// Created by chris on 4/17/2025.
//

#include "CliRadioManager.h"

#include "../include/cli/BaseFlag.h"
#include "../include/cli/SimpleFlag.h."
#include "../include/cli/ArgumentFlag.h"
#include "../include/RadioPacketDefinitions.h"

/* Radio instance */
//RFM9xRadio radio;

/* Callback functions */
void callback(uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid) {
//    uint8_t packetBuffer[MAX_PACKET_SIZE];
//
//    // filling the radioPacket
//    RadioPacketHeader headerPacket{};
//    headerPacket.packetNumber = 0;
//    headerPacket.packetLength = length;
//    headerPacket.groupId = group_uid;
//    headerPacket.flagId = flag_uid;
//
//    // total packet size
//    uint32_t totalPacketSize = sizeof(RadioPacketHeader) + length;
//
//    // handle oversized packets
//    if (totalPacketSize > MAX_PACKET_SIZE) {
//        return;
//    }
//
//    memcpy(packetBuffer + sizeof(RadioPacketHeader), data, length);
//
//    radio.transmit(packetBuffer, totalPacketSize);
}

CliRadioManager::CliRadioManager(Parser &parser, RadioLink &radio) : m_parserObject(parser),
                                                                     m_radioObject(radio) { }


void CliRadioManager::startCliLoop(char *input) {
//    m_parserObject.parse(input);
//    m_parserObject.runFlags();
}
