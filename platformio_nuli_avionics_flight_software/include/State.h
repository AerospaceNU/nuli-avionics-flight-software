#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_STATE_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_STATE_H

enum States_e {
    PRE_LAUNCH,
    MOTOR_ASCENT,
    BURNOUT_ASCENT,
    DESCENT,
    DESCENT_DEPLOY_MAIN,
    POST_FLIGHT,
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_STATE_H
