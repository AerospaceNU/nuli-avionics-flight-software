#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_COMMONSTRUCTS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_COMMONSTRUCTS_H

struct RawSensorData {

};

struct FilteredSensorData {

};

struct ActiveEvents {

};

struct Messages {
    char* messages[4];
};

struct Responses {
    char* messages[4];
};

enum State {
    IDLE,
    RUNNING,
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_COMMONSTRUCTS_H
