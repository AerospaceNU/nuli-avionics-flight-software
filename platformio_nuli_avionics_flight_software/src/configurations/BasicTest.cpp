// #include "Arduino.h"
//
// void setup() {
//     Serial.begin(9600);
//     while (!Serial) {}
// }
//
// void loop() {
//     Serial.println("Hello World");
//     delay(1000);
// }

#include <Arduino.h>

float noise(float x) {
    return (sin(x * 1.73) + cos(x * 2.13)) * 0.1;
}

float triangleWave(float t, float period) {
    float phase = fmod(t, period) / period;
    if (phase < 0.5) {
        return phase * 4 - 1; // rising from -1 to +1
    } else {
        return 3 - phase * 4; // falling from +1 to -1
    }
}

float randomWalkVal = 0.0;

void setup() {
    Serial.begin(9600);
    while (!Serial) {}
}

unsigned long lastTime = 0;

void loop() {
    unsigned long now = millis();
    float t = now / 1000.0;

    // Update random walk smoothly
    randomWalkVal += (random(-10, 11) / 100.0); // step between -0.1 and 0.1
    randomWalkVal = constrain(randomWalkVal, -1.0, 1.0);

    // Generate signals
    float sineVal = sin(2 * 3.14159 * 0.5 * t) + noise(t);
    float triangleVal = triangleWave(t, 4.0);
    float cosVal = cos(2 * 3.14159 * 0.2 * t) + noise(t * 0.7);

    // Print CSV line: sine, triangle, randomWalk, cosine
    Serial.print(sineVal, 4);
    Serial.print(",");
    Serial.print(triangleVal, 4);
    Serial.print(",");
    Serial.print(randomWalkVal, 4);
    Serial.print(",");
    Serial.println(cosVal, 4);

    // Echo any received data with extra message
    if (Serial.available()) {
        String received = Serial.readStringUntil('\n');
        Serial.print("Echo: ");
        Serial.print(received);
        Serial.println(" [Received your message!]");
    }

    delay(100); // ~10 Hz
}
