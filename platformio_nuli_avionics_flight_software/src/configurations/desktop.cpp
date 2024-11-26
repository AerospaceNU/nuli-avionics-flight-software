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

Barometer barometer;
Accelerometer accelerometer;
Gyroscope gyroscope;
Magnetometer magnetometer;

// Core objects accessible by all components
HardwareAbstraction hardware;
Configuration configuration;
Logger logger;
Filters filter;
AvionicsCore avionicsCore;

int main(int argc, char *argv[]) {
    CustomCsvParser parserObject = CustomCsvParser();
    std::string inputFile = R"(C:\Users\chris\Documents\AerospaceNU\nuli-avionics-flight-software\platformio_nuli_avionics_flight_software\src\drivers\desktop\output-post.csv)";
    if (parserObject.parse(inputFile, true) < 0) {
        printf("Failed\n");
        return -1;
    }

    for (aeroCsvRow_s row : parserObject.m_csv) {
        printf("Timestamp ms: %u, imu2 accelerometer: %.11lf\n", row.timestamp_s, row.baro1_pres);
    }



    hardware.addBarometer(&barometer);
    hardware.addAccelerometer(&accelerometer);
    hardware.addGyroscope(&gyroscope);
    hardware.addMagnetometer(&magnetometer);



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


//    while (true) {
//        avionicsCore.loopOnce();
//        accelerometer.inject({0, 0, 0}, 0);
    }
}