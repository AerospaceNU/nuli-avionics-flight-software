/*
  RadioChatSX1262.cpp -- LoRa text chat node on the NULI Radio Board.

  Hardware: SAMD51J19A + Semtech SX1262, using the custom radio_board variant
  (SPI on SERCOM0). Pairs with RadioChatFeatherM0.cpp on a Feather M0 RFM95.

  Board specifics handled here:
    - TCXO (Y2) is powered from the 3V3 rail, NOT from the SX1262 DIO3, so we
      pass tcxoVoltage = 0 to stop RadioLib driving DIO3 as a TCXO supply.
    - DIO2 drives the antenna TX/RX switch -> setDio2AsRfSwitch(true).

  Env: [env:RadioChatSX1262]
*/

#include "RadioChatCommon.h"

// CS/NSS, DIO1 (IRQ), RESET, BUSY -- names come from the radio_board variant.
// Cast to uint32_t so PIN_RADIO_NSS (== 0u, a null-pointer constant) doesn't
// make the Module(RadioLibHal*, ...) overload ambiguous.
static SX1262 radio = new Module(static_cast<uint32_t>(PIN_RADIO_NSS),
                                 static_cast<uint32_t>(PIN_RADIO_DIO1),
                                 static_cast<uint32_t>(PIN_RADIO_RESET),
                                 static_cast<uint32_t>(PIN_RADIO_BUSY));

void setup() {
    Serial.begin(115200);
    while (!Serial) { /* wait for USB serial monitor */ }
    Serial.println();
    Serial.println(F("=== Radio Chat :: SX1262 (NULI Radio Board) ==="));

    // SX1262 maximum output power.
    constexpr int8_t TX_POWER = 22; // dBm

    // tcxoVoltage = 0.0 -> external always-on TCXO; useRegulatorLDO = false -> DC-DC.
    int state = radio.begin(radiochat::FREQ, radiochat::BW, radiochat::SF,
                            radiochat::CR, radiochat::SYNC, TX_POWER,
                            radiochat::PREAMBLE, 0.0, false);
    if (state != RADIOLIB_ERR_NONE) radiochat::halt("begin", state, PIN_LED);

    state = radio.setDio2AsRfSwitch(true);
    if (state != RADIOLIB_ERR_NONE) radiochat::halt("setDio2AsRfSwitch", state, PIN_LED);

    radio.setPacketReceivedAction(radiochat::onRx);
    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) radiochat::halt("startReceive", state, PIN_LED);

    Serial.println(F("Ready. Type a message + Enter to send."));
}

void loop() {
    radiochat::poll(radio);
}
