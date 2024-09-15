#include <Arduino.h>

void setup() {
    // write your initialization code here
    Serial.begin(9600);
}

void loop() {
    Serial.println("hi");
    Serial.print(Serial.read());
    delay(1000);
    // write your code here
}