//#include "Parser.h"
//#include "SimpleFlag.h"
//#include "ArgumentFlag.h"
//#include "CLIEnums.h"

/*
--config
      -t int*1 Configure a trigger with additional flag:
      -m int*1 Trigger type (required)
               Type 1 = pyro
               Type 2 = line cutter
               Type 3 = digital on (on pyro)
               Type 4 = digital off (on pyro)
               Type 5 = pwm (on pyro)
      -p int*1 Pyro num or cut channel (required)
      -d float*1 Duration (required for pyro and pwm)
      -w float*1 Pulse width (required for pwm)
      -C string*1 Configuration using expression notation, required. Must be in quotes
      -D Delete this trigger
      -N Disallow manual triggering
 -e float*1 Configure ground elevation (in meters)
 -r float*1 Configure ground temperature (in Celsius)
 -c int*1 Configure radio channel (in multiples of bandwidth), negative numbers allowed
 -h Help for config. Prints current configuration values

--create_flight  Clear state log and move back to preflight

--erase  Fully erases on-board flash

--help  FCB produces standard command line help string

--linecutter -i int*1 Send linecutter cut with given ID (required)
 -c string*1 Send a command (required)

--offload  Offloads the last flight recorded on the board
 -f int*1 Offload a specific flight number off the board
 -h Help for offload. Prints info about each flight

--triggerfire -t int*1 IMMEDIATELY fires the given trigger number

--sense  Reads back most recent sensor data

--sim  Simulate past flights in hardware

--version  Send Git version and tag info
 */

#include "../src/core/cli/SimpleFlag.h"
#include "../src/core/cli/ArgumentFlag.h"
#include "../src/core/cli/Parser.h"
#include "../src/core/cli/CLIEnums.h"

/* Callback functions */
void boolCallback(uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid) {
    printf("Group UID: %u, Flag UID: %u\n", group_uid, flag_uid);

    if (length >= sizeof(bool)) {
        bool value = false;
        memcpy(&value, data, sizeof(bool));

        printf("Received boolean value: %s\n", value ? "true" : "false");
    } else {
        printf("Error: Insufficient data for boolean type\n");
    }
}

void intCallback(uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid) {
    printf("Group UID: %u, Flag UID: %u\n", group_uid, flag_uid);

    if (length >= sizeof(int)) {
        // Extract the integer value from the data buffer
        int value = 0;
        memcpy(&value, data, sizeof(int));

        printf("Received integer value: %d\n", value);
    } else {
        printf("Error: Insufficient data for integer type\n");
    }
}

void floatCallback(uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid) {
    printf("Group UID: %u, Flag UID: %u\n", group_uid, flag_uid);

    if (length >= sizeof(float)) {
        // Extract the float value from the data buffer
        float value = 0.0f;
        memcpy(&value, data, sizeof(float));

        printf("Received float value: %f\n", value);
    } else {
        printf("Error: Insufficient data for float type\n");
    }
}

void stringCallback(uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid) {
    printf("Group UID: %u, Flag UID: %u\n", group_uid, flag_uid);

    if (data && length > 0) {
        // Create a null-terminated copy of the string
        char* str = (char*)malloc(length + 1);
        if (str) {
            memcpy(str, data, length);
            str[length] = '\0';  // Ensure null termination

            printf("Received string value: \"%s\"\n", str);

            free(str);
        } else {
            printf("Error: Memory allocation failed\n");
        }
    } else {
        printf("Error: No string data received\n");
    }
}

void doubleCallback(uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid) {
    printf("Group UID: %u, Flag UID: %u\n", group_uid, flag_uid);

    if (length >= sizeof(double)) {
        // Extract the double value from the data buffer
        double value = 0.0;
        memcpy(&value, data, sizeof(double));

        printf("Received double value: %f\n", value);
    } else {
        printf("Error: Insufficient data for double type\n");
    }
}

void generalCallback(const char *name, uint8_t *data, uint32_t length, uint8_t group_uid, uint8_t flag_uid,
                          BaseFlag *dependency) { }

int main() {
    Parser myParser = Parser();

    /* config group */
    SimpleFlag config("--config", "Configure a trigger with additional flag", true, CONFIG_STARTER, generalCallback);
    ArgumentFlag<int> config_trigger("-t", "Trigger to configure", false, CONFIG_TRIGGER, generalCallback);
    ArgumentFlag<int> config_triggerType("-m", 2, "Trigger type", false, CONFIG_TRIGGER_TYPE, generalCallback);
    ArgumentFlag<int> config_pyro("-p", "Pyro num or cut channel", false, CONFIG_PYRO, generalCallback);
    ArgumentFlag<float> config_duration("-d", "Duration", false, CONFIG_DURATION, generalCallback);
    ArgumentFlag<float> config_pulse("-w", "Pulse width", false, CONFIG_PULSE, generalCallback);
    ArgumentFlag<const char*> config_configuration("-C", "Configuration using expression notation. Must be in quotes", false, CONFIG_CONFIGURATION, generalCallback);
    SimpleFlag config_deleteT("-D", "Delete this trigger", false, CONFIG_DELETE, generalCallback);
    SimpleFlag config_manual("-N", "Disallow manual triggering", false, CONFIG_MANUAL, generalCallback);
    ArgumentFlag<float> config_elevation("-e", "Configure ground elevation (in meters)", false, CONFIG_ELEVATION, generalCallback);
    ArgumentFlag<float> config_groundTemp("-r", "Configure ground temperature (in celsius)", false, CONFIG_GROUND_TEMP, generalCallback);
    ArgumentFlag<int> config_channel("-c","Configure radio channel (in multiples of bandwidth), negative numbers allowed", false, CONFIG_CHANNEL, generalCallback);
    BaseFlag* configGroup[]{&config, &config_trigger, &config_triggerType,
                            &config_pyro, &config_duration, &config_pulse,
                            &config_configuration, &config_deleteT,
                            &config_manual, &config_elevation,
                            &config_groundTemp, &config_channel};

    /* create group */
    SimpleFlag createFlight("--create_flight", "Clear state log and move back to preflight", true, 255, generalCallback);
    BaseFlag* createFlightGroup[](&createFlight);

    /* erase group */
    SimpleFlag erase("--erase", "Fully erases on-board flash", true, 255, generalCallback);
    BaseFlag* eraseGroup[](&erase);

    /* help group */
    SimpleFlag help("--help", "FCB produces standard command line help string", true, 255, generalCallback);
    BaseFlag* helpGroup[](&help);

    /* linecutter group */
    SimpleFlag linecutter("--linecutter", "Send linecutter cut with given", true, 255, generalCallback);
    ArgumentFlag<int> linecutter_id("-i", "Send linecutter cut with given ID", true, 255, generalCallback);
    ArgumentFlag<const char*> linecutter_command("-c", "Send a command", true, 255, generalCallback);
    BaseFlag* linecutterGroup[](&linecutter, &linecutter_id, &linecutter_command);

    /* offload group */
    SimpleFlag offload("--offload", "Offloads the last flight recorded on the board", true, 255, generalCallback);
    ArgumentFlag<int> offload_flightNum("-f", "Offload a specific flight number off the board", false, 255, generalCallback);
    SimpleFlag offload_help("-h", "Help for offload. Prints info about each flight", true, 255, generalCallback);
    BaseFlag* offloadGroup[](&offload, &offload_flightNum, &offload_help);

    /* triggerfire group */
    SimpleFlag triggerfire("--triggerfire", "IMMEDIATELY fires the given trigger number", true, 255, generalCallback);
    ArgumentFlag<int> triggerfire_triggerNum("-t", "Trigger number to fire", true, 255, generalCallback);
    BaseFlag* triggerfireGroup[](&triggerfire, &triggerfire_triggerNum);

    /* sense group */
    SimpleFlag sense("--sense", "Reads back most recent sensor data", true, 255, generalCallback);
    BaseFlag* senseGroup[](&sense);

    /* sim group */
    SimpleFlag sim("--sim", "Simulate past flights in hardware", true, 255, generalCallback);
    BaseFlag* simGroup[](&sim);

    /* version group */
    SimpleFlag version("--version", "Send Git version and tag info", true, 255, generalCallback);
    BaseFlag* versionGroup[](&version);


    // adding to parser
    myParser.addFlagGroup(configGroup, CONFIG);                 // <--- increment from last set uid
    myParser.addFlagGroup(createFlightGroup, CREATE_FLIGHT);    // <--- manually set uid
    myParser.addFlagGroup(eraseGroup, ERASE);
    myParser.addFlagGroup(helpGroup, HELP);
    myParser.addFlagGroup(linecutterGroup, LINECUTTER);
    myParser.addFlagGroup(offloadGroup, OFFLOAD);
    myParser.addFlagGroup(triggerfireGroup, TRIGGERFIRE);
    myParser.addFlagGroup(senseGroup, SENS);
    myParser.addFlagGroup(simGroup, SIM);
    myParser.addFlagGroup(versionGroup, VERSION);

    myParser.printHelp();

    while(true) {

        char input[64] = {0};
        fgets(input, 64, stdin);

        // parses user input
        if (myParser.parse(input) != CLI_SUCCESS) {
            continue;
        }

        printf("Doing callbacks!\n");
        myParser.runFlags();

        // resetting all flags to default values.
        // --> all flags are unset
        // --> all inputted data is reset to nothing (theoretically, instead just isSet is set to false)
        myParser.resetFlags();
    }

}