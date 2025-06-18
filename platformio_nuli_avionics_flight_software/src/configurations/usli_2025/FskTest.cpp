include <Arduino.h>
#include "AprsTransmitter.h"

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
    AprsTransmitter::setup();
}

void loop() {
//    delay(1000);
    Serial.println("starting");

    // Send the data
//    uint8_t data[] = {0b10101010, 0b11110000, 0b00111010};
//    uint8_t data[] = {0b01111110, 0b01111110, 0b01111110, 0b10001100, 0b10001100, 0b00111110, 0b01011011, 0b10001101, 0b10011011, 0b10001101};
    uint8_t data[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 43, 83, 108, 236, 169, 86, 174, 196, 220, 235, 43, 68, 187, 46, 243, 36, 180, 203, 110, 169, 209, 213, 95, 2, 176, 200, 239, 79, 46, 145, 104, 200, 139, 186, 127, 127, 127, 0};
//    uint8_t data[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 43, 83, 108, 236, 169, 86, 174, 196, 220, 235, 43, 68, 187, 46, 243, 36, 180, 203, 110, 169, 209, 213, 95, 2, 215, 16, 183, 119, 17, 78, 206, 236, 11, 192, 64, 64 };


    AprsTransmitter::send(&data[0], sizeof(data) * 8 - 7);
    while (AprsTransmitter::isCurrentlyTransmitting());

    Serial.println("done");

    delay(5000);

//    while (true);
}