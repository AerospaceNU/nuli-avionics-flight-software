#include "Arduino.h"


void setup() {
    pinMode(11, OUTPUT);
    pinMode(12, OUTPUT);
    pinMode(13, OUTPUT);
    digitalWrite(11, HIGH);
    digitalWrite(12, LOW);
    digitalWrite(13, LOW);
}

//125 = 4khz
int delay2k_us = 500;
int delay3p7k_us = 270;
int delay6kus = 166;
int delay_us = delay3p7k_us / 2;
uint32_t nextTime = 5000;

void loop() {
//    if (millis() > nextTime) {
//        nextTime = millis() + 5000;
//        delay_us -= 20;
//    }
//    if (delay_us < 30) {
//        delay_us = 300;
//    }

    digitalWrite(12, LOW);
    digitalWrite(13, HIGH);
    delayMicroseconds(delay_us);
    digitalWrite(12, HIGH);
    digitalWrite(13, LOW);
    delayMicroseconds(delay_us);
}
