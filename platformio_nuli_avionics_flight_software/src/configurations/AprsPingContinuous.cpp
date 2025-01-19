#include <Arduino.h>
#include "AprsModulation.h"

AprsModulation aprsModulation(A0, "AERONU");

char msg[200];
uint32_t nextTime = 0;

void setup() {
    Serial.begin(9600);
    aprsModulation.setup();
}

/**
 * Callsign;time;temperature;apogee;ort degs;max vel ms;landing velocity*10;g forces (g)*10;survibility
 * NULI;00009;23;4500;86;102;51;12;97
 *
 */

char msg2[200];
char *str = nullptr;

void begin(const char *callsign) {
    str = msg2;
    str += sprintf(str, "%s", callsign);
}

void addInt(int num) {
    str += sprintf(str, ";%d", num);
}

void sendUSLIMessage(int time, int temp, int alt, int ort, int maxVel, int landVel, int accel, int suviv) {
    begin("NULI");
    addInt(time);
    addInt(temp);
    addInt(alt);
    addInt(ort);
    addInt(maxVel);
    addInt(landVel);
    addInt(accel);
    addInt(suviv);
    aprsModulation.transmit(msg2);
}


void loop() {
    Serial.println(millis());
    if (millis() > nextTime) {
        nextTime = millis() + 2000;
        sendUSLIMessage(millis() / 1000, 23, 4500, 87, 102, 51, 12, 97);
//        sprintf(msg, "NULI Transmitter;   Time (s): %d", int(millis() / 1000));
//        aprsModulation.transmit(msg);
    }
}