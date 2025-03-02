//
// Created by chris on 2/12/2025.
//
//
// Created by chris on 2/12/2025.
//


#include "Parser.h"
#include "SimpleFlag.h"
#include "ArgumentFlag.h"

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

int main() {
    Parser myParser = Parser(stdin, stdout, stderr);

    /* config group */
    SimpleFlag config("--config", "Configure a trigger with additional flag", true);
    ArgumentFlag<int> config_trigger("-t", "Trigger to configure", false);
    ArgumentFlag<int> config_triggerType("-m", "Trigger type", false);
    ArgumentFlag<int> config_pyro("-p", "Pyro num or cut channel", false);
    ArgumentFlag<float> config_duration("-d", "Duration", false);
    ArgumentFlag<float> config_pulse("-w", "Pulse width", false);
    ArgumentFlag<const char*> config_configuration("-C", "Configuration using expression notation. Must be in quotes", false);
    SimpleFlag config_deleteT("-D", "Delete this trigger", false);
    SimpleFlag config_manual("-N", "Disallow manual triggering", false);
    ArgumentFlag<float> config_elevation("-e", "Configure ground elevation (in meters)", false);
    ArgumentFlag<float> config_groundTemp("-r", "Configure ground temperature (in celsius)", false);
    ArgumentFlag<int> config_channel("-c","Configure radio channel (in multiples of bandwidth), negative numbers allowed", false);
    BaseFlag* configGroup[]{&config, &config_trigger, &config_triggerType,
                            &config_pyro, &config_duration, &config_pulse,
                            &config_configuration, &config_deleteT,
                            &config_manual, &config_elevation,
                            &config_groundTemp, &config_channel};

    /* create group */
    SimpleFlag createFlight("--create_flight", "Clear state log and move back to preflight", true);
    BaseFlag* createFlightGroup[](&createFlight);

    /* erase group */
    SimpleFlag erase("--erase", "Fully erases on-board flash", true);
    BaseFlag* eraseGroup[](&erase);

    /* help group */
    SimpleFlag help("--help", "FCB produces standard command line help string", true);
    BaseFlag* helpGroup[](&help);

    /* linecutter group */
    SimpleFlag linecutter("--linecutter", "Send linecutter cut with given", true);
    ArgumentFlag<int> linecutter_id("-i", "Send linecutter cut with given ID", true);
    ArgumentFlag<const char*> linecutter_command("-c", "Send a command", true);
    BaseFlag* linecutterGroup[](&linecutter, &linecutter_id, &linecutter_command);

    /* offload group */
    SimpleFlag offload("--offload", "Offloads the last flight recorded on the board", true);
    ArgumentFlag<int> offload_flightNum("-f", "Offload a specific flight number off the board", false);
    SimpleFlag offload_help("-h", "Help for offload. Prints info about each flight", true);
    BaseFlag* offloadGroup[](&offload, &offload_flightNum, &offload_help);

    /* triggerfire group */
    SimpleFlag triggerfire("--triggerfire", "IMMEDIATELY fires the given trigger number", true);
    ArgumentFlag<int> triggerfire_triggerNum("-t", "Trigger number to fire", true);
    BaseFlag* triggerfireGroup[](&triggerfire, &triggerfire_triggerNum);

    /* sense group */
    SimpleFlag sense("--sense", "Reads back most recent sensor data", true);
    BaseFlag* senseGroup[](&sense);

    /* sim group */
    SimpleFlag sim("--sim", "Simulate past flights in hardware", true);
    BaseFlag* simGroup[](&sim);

    /* version group */
    SimpleFlag version("--version", "Send Git version and tag info", true);
    BaseFlag* versionGroup[](&version);


    // adding to parser
    myParser.addFlagGroup(configGroup);
    myParser.addFlagGroup(createFlightGroup);
    myParser.addFlagGroup(eraseGroup);
    myParser.addFlagGroup(helpGroup);
    myParser.addFlagGroup(linecutterGroup);
    myParser.addFlagGroup(offloadGroup);
    myParser.addFlagGroup(triggerfireGroup);
    myParser.addFlagGroup(senseGroup);
    myParser.addFlagGroup(simGroup);
    myParser.addFlagGroup(versionGroup);

    while(true) {
//        myParser.printHelp();

        char input[64] = {0};
        fgets(input, 64, stdin);

        // parses user input
        if (myParser.parse(input) < 0) {
            continue;
        }

        printf("Parsed!\n");

        // set 1
        // when retrieving a value, you should always check if the flag is set.
        // otherwise, undefined behaviour can occur.
        bool returnBool;
        if (myParser.getValue<bool>(config.name(), config.name(), returnBool) < 0) {
            continue;
        }
        printf("Config set: %d\n", returnBool);

        int returnInt;
        if (myParser.getValue<int>(config.name(), config_trigger.name(), returnInt) < 0) {
            printf("get value failed\n");
            continue;
        }
        printf("Trigger Num is: %d\n", returnInt);

        const char* returnChar;
        if (myParser.getValue<const char*>(config.name(), config_configuration.name(), returnChar) < 0) {
            continue;
        }
        printf("Configuration is: %s\n\n", returnChar);


        // set 2, should be same as set 1
        bool config_set = config.getValue<bool>();
        printf("Config set: %d\n", config_set);

        int triggerNum = config_trigger.getValue<int>();
        printf("Trigger Num is: %d\n", triggerNum);

        const char* configuration = config_configuration.getValue<const char*>();
        printf("Configuration is: %s\n", configuration);

        // resetting all flags to default values.
        // --> all flags are unset
        // --> all inputted data is reset to nothing (theoretically, instead just isSet is set to false)
        myParser.resetFlags();
    }

}