#include <Arduino.h>
#include "drivers/samd21/AprsModulation.h"

char message[3000];
AprsModulation aprsModulation(A0, "AERONU");

void setup() {
    // Start APRS
    aprsModulation.setup();
    // Start Serial
    Serial.begin(9600);
    while (!Serial);
    Serial.println("Please enter a message:");
}

void loop() {
    if (Serial.available()) {
        for (int i = 0; true;) {
            if (Serial.available()) {
                char c = Serial.read();
                // Check if we have reached the end of a message
                if (c == '\r' || c == '\n') {
                    // Terminate the message
                    message[i] = '\0';
                    // Remove the new line char if terminated with \r\n
                    if (Serial.peek() == '\r' || Serial.peek() == '\n') Serial.read();
                    // Exit loop
                    break;
                }
                // Record the character
                message[i] = c;
                i++;
            }
        }
        Serial.print("New message: ");
        Serial.println(message);

        // Send the message
        aprsModulation.transmit(message);
        Serial.println();

        Serial.println("Please enter a message:");
    }
}