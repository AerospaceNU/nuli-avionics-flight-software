#include <Arduino.h>

void setup() {
//    Serial.begin(9600);
////    while (!Serial);
//    Serial.println("test");
//    Serial1.begin(9600);

    pinMode(1, OUTPUT);
}

//volatile int asafd = 214;

void loop() {
    digitalWrite(1, HIGH);
    tone(0, 4000);
    delay(1000);
    digitalWrite(1, LOW);
    noTone(0);
    delay(1000);

//    asafd += 1;
//    volatile int a = asafd;
//    a += 1;
//    Serial.println(millis());
//    delay(1000);
}