#include <Arduino.h>

void setup() {
    Serial.begin(9600);
//    while (!Serial);
    Serial.println("test");
    Serial1.begin(9600);

//    pinMode(LED_BUILTIN, OUTPUT);
}

volatile int asafd = 214;

void loop() {
    asafd += 1;
    volatile int a = asafd;
    a += 1;
    Serial.println(millis());
    delay(1000);
}