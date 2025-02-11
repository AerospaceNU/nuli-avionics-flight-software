#include <Avionics.h>
#include <GenericSensor.h>
#include <Barometer.h>
#include <Accelerometer.h>
#include <GPS.h>
#include <Gyroscope.h>
#include <Magnetometer.h>
#include <Pyro.h>
#include <AvionicsCore.h>
#include <HardwareAbstraction.h>
#include <Filters.h>
#include <FlashMemory.h>
#include <CommunicationLink.h>
#include "CustomCsvParser.h"
#include "cli/Parser.h"

#include <iostream>
#include <DesktopDebug.h>

Barometer barometer;
Accelerometer accelerometer;
Gyroscope gyroscope;
Magnetometer magnetometer;

DesktopDebug desktopDebug;

// Core objects accessible by all components
HardwareAbstraction hardware;
Configuration configuration;
Logger logger;
Filters filter;
AvionicsCore avionicsCore;

int main(int argc, char *argv[]) {
    while(true) {
        // parser object
        Parser myParser = Parser();

        /* Group 1 */
        SimpleFlag config("--config", "Configure a trigger with additional flags_s", true);
        ArgumentFlag<int> config_trigger("-t", 5, "Trigger number", true);
        ArgumentFlag<int> config_triggerType("-m", 0, "Trigger type\n    Type 1 = pyro\n    Type 2 = line cutter", false);
        ArgumentFlag<int> config_pyro("-p", 0, "Pyro num or cut channel (required)", false);
        ArgumentFlag<float> config_duration("-d", 0.0, "Duration (required for pyro and pwm)", false);
        ArgumentFlag<float> config_pulseWidth("-w", 0.0, "Pulse width (required for pwm)", false);
        ArgumentFlag<const char*> config_elevation("-e", "Configure ground elevation (in meters)", false);
        SimpleFlag temp("-b", "just a temp, delete", true);

        BaseFlag* configGroup[]{&config, &config_trigger, &config_triggerType, &config_pyro, &config_duration, &config_pulseWidth, &config_elevation, &temp};
        myParser.addFlagGroup(configGroup);

        /* Group 2 */
        SimpleFlag create("--create_flight", "Clear state log and move back to preflight", false);

        BaseFlag* createFlightGroup[]{&create};
        myParser.addFlagGroup(createFlightGroup);

        /* Group 3 */
        ArgumentFlag<int> erase("--erase", "Fully erases on-board flash", false);
        SimpleFlag flight("-f", "Clear state log and move back to preflight", false);

        BaseFlag* eraseGroup[]{&erase, &flight};
        myParser.addFlagGroup(eraseGroup);

        /* Group 4*/
        ArgumentFlag<int> testD_1("--testD", "some help text", true);
        ArgumentFlag<int> testD_2("-a", "some help text", true);
        BaseFlag* testGroupD[] = {&testD_1, &testD_2};
        myParser.addFlagGroup(testGroupD);



        char input[64] = {0};
        fgets(input, 64, stdin);
        printf("\nRead: %s\n", input);

        // parsing input
        if (myParser.parse(input) < 0)
            ;

        printf("Is erase set? %d\n", erase.isSet());
        printf("Erase is %d\n", erase.getValueDerived());

        myParser.printHelp();
        myParser.resetFlags();
    }


//    CustomCsvParser dataCsv = CustomCsvParser();
//    std::string inputFile = R"(C:\Users\chris\Documents\AerospaceNU\nuli-avionics-flight-software\platformio_nuli_avionics_flight_software\src\drivers\desktop\output-post.csv)";
//    if (dataCsv.parse(inputFile, true) < 0) {
//        printf("Failed\n");
//        return -1;
//    }
//
//
//    hardware.addBarometer(&barometer);
//    hardware.addAccelerometer(&accelerometer);
//    hardware.addGyroscope(&gyroscope);
//    hardware.addMagnetometer(&magnetometer);
//
//    avionicsCore.setup(&hardware, &configuration, &logger, &filter);
//
//
//
//    /**
//     * Access CSV
//     * Get data from the accel_x,y,z
//     * On each tick,
//     * Inject into accelerometer
//     * Eventually inject into all possible/available sensors
//     * Then go to next row of data next time (basically keep track)
//     * https://drive.google.com/drive/folders/11GcJTWOurP-Bj4_6t0MK_s8RIyoK1KlT
//     */
//
//
//    // inject data values into simulation
//    // reset once simulation completes
////    while(true) {
//        for (size_t i = 0; i < dataCsv.getSize(); ++i) {
//            avionicsCore.loopOnce();
//            CustomCsvParser::FlightDataRow_s row = dataCsv.getRow(i);
//
//            barometer.inject(row.baro1_s.temp, 0, row.baro1_s.pres);
//            accelerometer.inject({row.imu1_accel_s.x, row.imu1_accel_s.y, row.imu1_accel_s.z}, row.baro1_s.temp);
//            gyroscope.inject({row.imu1_gyro_s.x, row.imu1_gyro_s.y, row.imu1_gyro_s.z}, row.baro1_s.temp);
//            magnetometer.inject({row.imu1_mag_s.x, row.imu1_mag_s.y, row.imu1_mag_s.z}, row.baro1_s.temp);
//        }
////    }
}