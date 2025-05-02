//
// Created by chris on 4/14/2025.
//

#ifndef DESKTOP_CLIENUMS_H
#define DESKTOP_CLIENUMS_H

enum FlagGroupEnum_e {
    PING = 0,
    DEPLOY = 1,
    ERASE = 2,
    START_LOGGING = 3,
    STOP_LOGGING = 4,
    TRANSMIT = 5,
    GPS = 6,
    STRING = 255,

    CONFIG = 0,
    CREATE_FLIGHT = 1,
//    ERASE = 2,
    HELP = 3,
    LINECUTTER = 4,
    OFFLOAD = 5,
    TRIGGERFIRE = 6,
    SENS = 7,
    SIM = 8,
    VERSION = 9,
};

enum ConfigCommands_e {
    CONFIG_STARTER = 0,
    CONFIG_TRIGGER = 1,
    CONFIG_TRIGGER_TYPE = 2,
    CONFIG_PYRO = 3,
    CONFIG_DURATION = 4,
    CONFIG_PULSE = 5,
    CONFIG_CONFIGURATION = 6,
    CONFIG_DELETE = 7,
    CONFIG_MANUAL = 8,
    CONFIG_ELEVATION = 9,
    CONFIG_GROUND_TEMP = 10,
    CONFIG_CHANNEL = 11,
};

enum PayloadData_e {
    END = 1,
    CONTINUE = 2,
};

enum ConfigConfiguration_e {
    CONFIG_CONFIGURATION_APOGEE = 0,
    CONFIG_CONFIGURATION_SOMETHING = 1,
};

#endif //DESKTOP_CLIENUMS_H
