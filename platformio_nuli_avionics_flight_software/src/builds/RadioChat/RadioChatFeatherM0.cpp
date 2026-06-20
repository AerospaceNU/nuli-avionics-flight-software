/*
  RadioChatFeatherM0.cpp -- LoRa text chat node on an Adafruit Feather M0 RFM95.

  Hardware: SAMD21G18A + RFM95 (Semtech SX1276). Pairs with RadioChatSX1262.cpp
  on the NULI Radio Board.

  Standard Adafruit Feather M0 RFM95 wiring (https://learn.adafruit.com/...):
    CS = 8, RST = 4, DIO0 (IRQ) = 3.
  DIO1 is not needed for basic LoRa RX/TX (and is unconnected unless bridged on
  the board), so it is left as RADIOLIB_NC.

  Env: [env:RadioChatFeatherM0]
*/

#include "RadioChatCommon.h"

#define RFM95_CS    8
#define RFM95_DIO0  3   // IRQ
#define RFM95_RST   4

// CS, IRQ (DIO0), RESET, GPIO (DIO1 unused -> NC)
static SX1276 radio = new Module(RFM95_CS, RFM95_DIO0, RFM95_RST, RADIOLIB_NC);

void setup() {
    Serial.begin(115200);
    while (!Serial) { /* wait for USB serial monitor */ }
    Serial.println();
    Serial.println(F("=== Radio Chat :: SX1276 (Feather M0 RFM95) ==="));

    // RFM95/SX1276 maximum output power (PA_BOOST + PA_DAC).
    constexpr int8_t TX_POWER = 20; // dBm

    int state = radio.begin(radiochat::FREQ, radiochat::BW, radiochat::SF,
                            radiochat::CR, radiochat::SYNC, TX_POWER,
                            radiochat::PREAMBLE);
    if (state != RADIOLIB_ERR_NONE) radiochat::halt("begin", state, LED_BUILTIN);

    radio.setPacketReceivedAction(radiochat::onRx);
    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) radiochat::halt("startReceive", state, LED_BUILTIN);

    Serial.println(F("Ready. Type a message + Enter to send."));
}

void loop() {
    radiochat::poll(radio);
}
