#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DEBUGSTREAM_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DEBUGSTREAM_H

#include "Avionics.h"
#include "ConstantsUnits.h"

class DebugStream {
public:
    virtual void setup() {

    }

    virtual void print(const char *str) {

    }

    void print(char c) {
        char aa[2];
        aa[0] = c;
        aa[1] = 0;
        print(aa);
    }

    virtual void print(int64_t num) {

    }

    void print(uint64_t num) {
        print(int64_t(num));
    }

    virtual void print(double num) {

    }

    void println() {
        print("\n");
    }
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DEBUGSTREAM_H
