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

int main(int argc, char* argv[]) {
    CustomCsvParser dataCsv = CustomCsvParser();
    std::string inputFile = R"(C:\Users\chris\Documents\AerospaceNU\nuli-avionics-flight-software\platformio_nuli_avionics_flight_software\src\drivers\desktop\output-post.csv)";
    if (argc >= 2) {
        inputFile = argv[1];
        std::cout << inputFile << "\n";
    }
    if (dataCsv.parse(inputFile, true) < 0) {
        printf("Failed\n");
        return -1;
    }


    hardware.addDebugStream(&desktopDebug);
    hardware.addBarometer(&barometer);
    hardware.addAccelerometer(&accelerometer);
    hardware.addGyroscope(&gyroscope);
    hardware.addMagnetometer(&magnetometer);

    avionicsCore.setup(&hardware, &configuration, &logger, &filter);



    /**
     * @TODO
     * Access CSV
     * Get data from the accel_x,y,z
     * On each tick,
     * Inject into accelerometer
     * Eventually inject into all possible/available sensors
     * Then go to next row of data next time (basically keep track)
     * https://drive.google.com/drive/folders/11GcJTWOurP-Bj4_6t0MK_s8RIyoK1KlT
     */


    // inject data values into simulation
    // reset once simulation completes
    for (size_t i = 0; i < dataCsv.getSize(); ++i) {
        CustomCsvParser::FlightDataRow_s row = dataCsv.getRow(i);

        barometer.inject(row.baro1_s.temp, 0, row.baro1_s.pres);
        accelerometer.inject({row.imu1_accel_s.x, row.imu1_accel_s.y, row.imu1_accel_s.z}, row.baro1_s.temp);
        gyroscope.inject({row.imu1_gyro_s.x, row.imu1_gyro_s.y, row.imu1_gyro_s.z}, row.baro1_s.temp);
        magnetometer.inject({row.imu1_mag_s.x, row.imu1_mag_s.y, row.imu1_mag_s.z}, row.baro1_s.temp);

        avionicsCore.loopOnce();
        avionicsCore.printDump();
    }
}