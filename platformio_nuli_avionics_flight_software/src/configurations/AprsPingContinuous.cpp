#include <Arduino.h>
#include "drivers/arduino/aprs/AprsModulation.h"

AprsModulation aprsModulation(A0, "AERONU");

char msg[200];
uint32_t nextTime = 0;

void setup() {
    aprsModulation.setup();
}

void loop() {
    if (millis() > nextTime) {
        nextTime = millis() + 10000;
        sprintf(msg, "NULI Transmitter;   Time (s): %d", int(millis() / 1000));
        aprsModulation.transmit(msg);
    }
}