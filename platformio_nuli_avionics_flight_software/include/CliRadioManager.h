//
// Created by chris on 4/17/2025.
//

#ifndef DESKTOP_CLIRADIOMANAGER_H
#define DESKTOP_CLIRADIOMANAGER_H

#include "../include/cli/Parser.h"
#include "RadioLink.h"
#include "RFM9xRadio.h"

#define MAX_PACKET_SIZE 128

class CliRadioManager {
public:
    CliRadioManager(Parser &parser, RadioLink &radio);

    void startCliLoop(char *input);

private:
    Parser m_parserObject;
    RadioLink m_radioObject;


};


#endif //DESKTOP_CLIRADIOMANAGER_H
