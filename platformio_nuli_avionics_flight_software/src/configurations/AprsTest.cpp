#include <Arduino.h>
#include <AprsTransmitter.h>

/**
 * @todo pls make sure you can't transmit messages that are too long
 * @todo Make the max length constant in terms of bytes, remove length from struct and make global
 * @todo Remove the current datapoint variable and inline it
 * @todo Make relevant variables volatile
 * @todo Reduce sin table sizes
 * @todo Look into storing pointer offsets instead of pointer for each bit
 * @todo 
 */

void setup() {
    // Initialize the USB connection
    Serial.begin(9600);
//    while (!Serial);
//    Serial.println("starting");
//
//    // Send the data
//    uint8_t data[3] = {0b10101010, 0b11110000, 0b00111010};
//    AprsTransmitter::configure();
//    AprsTransmitter::send(&data[0], 3);
//    while (AprsTransmitter::isCurrentlyTransmitting());
//
//    Serial.println("done");
}

void loop() {
    Serial.println("starting");

    // Send the data
    uint8_t data[3] = {0b10101010, 0b11110000, 0b00111010};
    AprsTransmitter::configure();
    AprsTransmitter::send(&data[0], 3);
    while (AprsTransmitter::isCurrentlyTransmitting());

    Serial.println("done");

    delay(300);
}