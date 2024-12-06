#include <Arduino.h>
#include "AprsModulation.h"

AprsModulation aprsModulation(A0, "AERONU");

char msg[200];
uint32_t nextTime = 0;

void setup() {
    Serial.begin(9600);
    aprsModulation.setup();
}

void loop() {
    Serial.println(millis());
    if (millis() > nextTime) {
        nextTime = millis() + 10000;
        sprintf(msg, "NULI Transmitter;   Time (s): %d", int(millis() / 1000));
        aprsModulation.transmit(msg);
    }
}