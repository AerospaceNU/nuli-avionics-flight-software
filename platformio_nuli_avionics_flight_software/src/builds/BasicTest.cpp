#include "Arduino.h"

#define LED_PIN LED_BUILTIN

void setup() {
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    Serial.println(millis());
    delay(500);
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
}
