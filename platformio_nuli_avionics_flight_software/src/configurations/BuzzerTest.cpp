#include "Arduino.h"
#include "boards/SillyGoosePins.h"



void setup() {
    Serial.begin(9600);
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
}


void loop() {
    Serial.println("Hello World");
    digitalWrite(LIGHT_PIN, HIGH);
    tone(BUZZER_PIN, 3700);
    delay(500);
    digitalWrite(LIGHT_PIN, LOW);
    noTone(BUZZER_PIN);
    delay(500);
}
