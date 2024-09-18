#include <Arduino.h>
#include <AprsTransmitter.h>

void setup() {
    // Initialize the USB connection
    Serial.begin(9600);
    while (!Serial);
    Serial.println("starting");

    // Send the data
    uint8_t data[3] = {0b10101010, 0b11110000, 0b00111010};
    AprsTransmitter::configure();
    AprsTransmitter::send(&data[0], 3);
    while (AprsTransmitter::m_transmitActive);

    Serial.println("done");
}

void loop() {

}