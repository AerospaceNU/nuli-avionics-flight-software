/*
  RadioBeaconCommon.h -- shared on-air parameters for the one-way beacon test.

    - BeaconTxSX1262.cpp     transmits a counter message every 5 s (Radio Board)
    - BeaconRxFeatherM0.cpp  listens and prints whatever it hears (Feather M0)

  These MUST match between the two nodes or the Feather won't decode anything.
  (Same values as the RadioChat pair, so a chat node can also hear the beacon.)
*/

#pragma once

#include <Arduino.h>
#include <RadioLib.h>

namespace beacon {

constexpr float    FREQ     = 915.0;  // MHz
constexpr float    BW       = 125.0;  // kHz
constexpr uint8_t  SF       = 9;      // spreading factor
constexpr uint8_t  CR       = 7;      // coding rate 4/7
constexpr uint8_t  SYNC     = 0x12;   // private network sync word
constexpr uint16_t PREAMBLE = 8;      // symbols

// Fatal error: report and blink the LED forever.
static void halt(const char* what, int code, int ledPin) {
    Serial.print(F("[FATAL] "));
    Serial.print(what);
    Serial.print(F(" failed, code "));
    Serial.println(code);
    pinMode(ledPin, OUTPUT);
    while (true) {
        digitalWrite(ledPin, HIGH); delay(100);
        digitalWrite(ledPin, LOW);  delay(100);
    }
}

} // namespace beacon
