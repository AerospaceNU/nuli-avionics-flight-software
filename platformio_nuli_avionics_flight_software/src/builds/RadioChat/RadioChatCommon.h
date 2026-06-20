/*
  RadioChatCommon.h -- shared logic for a dead-simple two-node LoRa text chat.

  Type a line into the USB serial monitor + Enter -> it is transmitted.
  Anything received over the air is printed with RSSI / SNR.

  The same file backs both ends of the link:
    - RadioChatSX1262.cpp     (NULI Radio Board, SAMD51 + SX1262)
    - RadioChatFeatherM0.cpp  (Adafruit Feather M0 RFM95, SAMD21 + SX1276)

  The board-specific .cpp only constructs the radio and runs begin(); all the
  message plumbing lives here and operates on a RadioLib PhysicalLayer&, so both
  nodes behave identically. Only one .cpp is ever compiled per PlatformIO env,
  so the `static` definitions in this header are single-TU and safe.

  IMPORTANT: every on-air parameter below MUST be identical on both nodes or
  they will not hear each other. RadioLib maps the 1-byte sync word 0x12 to the
  SX126x 2-byte 0x1424, so SX1276 and SX1262 interoperate with the same value.
*/

#pragma once

#include <Arduino.h>
#include <RadioLib.h>

namespace radiochat {

// ---- on-air parameters (must match on both ends) ------------------------
constexpr float    FREQ     = 915.0;  // MHz (US ISM / board is 915 band)
constexpr float    BW       = 125.0;  // kHz
constexpr uint8_t  SF       = 9;      // spreading factor
constexpr uint8_t  CR       = 7;      // coding rate 4/7
constexpr uint8_t  SYNC     = 0x12;   // private network sync word
constexpr uint16_t PREAMBLE = 8;      // symbols
// NOTE: TX power is set per-board in each .cpp (it is transmit-side only and
// does not need to match): SX1262 maxes at +22 dBm, RFM95/SX1276 at +20 dBm.
// At max power, separate the boards and use antennas, or the RX front-end
// desenses and you get worse results than at low power.

// ---- receive interrupt flag ---------------------------------------------
static volatile bool rxFlag = false;
static void onRx() { rxFlag = true; }

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

static String g_input;

// Call every loop(): drains received packets and any typed serial line.
static void poll(PhysicalLayer& radio) {
    // -- something arrived over the air --
    if (rxFlag) {
        rxFlag = false;
        String msg;
        int state = radio.readData(msg);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.print(F("[RX] "));
            Serial.print(msg);
            Serial.print(F("   (RSSI "));
            Serial.print(radio.getRSSI());
            Serial.print(F(" dBm, SNR "));
            Serial.print(radio.getSNR());
            Serial.println(F(" dB)"));
        } else {
            Serial.print(F("[RX error, code "));
            Serial.print(state);
            Serial.println(F("]"));
        }
        radio.startReceive();
    }

    // -- the user typed something to send --
    while (Serial.available()) {
        char c = static_cast<char>(Serial.read());
        if (c == '\n' || c == '\r') {
            if (g_input.length() > 0) {
                Serial.print(F("[TX] "));
                Serial.println(g_input);
                int state = radio.transmit(g_input);
                if (state != RADIOLIB_ERR_NONE) {
                    Serial.print(F("  TX failed, code "));
                    Serial.println(state);
                }
                g_input = "";
                rxFlag = false;          // ignore self-trigger from the TX
                radio.startReceive();    // back to listening
            }
        } else {
            g_input += c;
        }
    }
}

} // namespace radiochat
