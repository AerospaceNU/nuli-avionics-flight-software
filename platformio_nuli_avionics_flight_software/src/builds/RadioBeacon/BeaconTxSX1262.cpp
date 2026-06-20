/*
  BeaconTxSX1262.cpp -- one-way LoRa beacon on the NULI Radio Board.

  Transmits an incrementing message every 5 seconds. Pair with
  BeaconRxFeatherM0.cpp on a Feather M0 RFM95 to watch the packets arrive.

  Board specifics (same as the chat build):
    - TCXO powered from 3V3, not DIO3  -> tcxoVoltage = 0
    - DIO2 drives the antenna switch    -> setDio2AsRfSwitch(true)

  Env: [env:BeaconTxSX1262]
*/

#include "RadioBeaconCommon.h"

// CS/NSS, DIO1 (IRQ), RESET, BUSY -- cast so PIN_RADIO_NSS (0u) isn't treated
// as a null-pointer constant (which makes the Module ctor ambiguous).
static SX1262 radio = new Module(static_cast<uint32_t>(PIN_RADIO_NSS),
                                 static_cast<uint32_t>(PIN_RADIO_DIO1),
                                 static_cast<uint32_t>(PIN_RADIO_RESET),
                                 static_cast<uint32_t>(PIN_RADIO_BUSY));

static uint32_t counter = 0;

void setup() {
    pinMode(PIN_LED, OUTPUT);

    Serial.begin(115200);
    // Don't block forever -- this node should beacon even with no monitor open.
    uint32_t t0 = millis();
    while (!Serial && (millis() - t0 < 2000)) { /* brief wait for the monitor */ }

    Serial.println();
    Serial.println(F("=== Beacon TX :: SX1262 (NULI Radio Board) ==="));

    constexpr int8_t TX_POWER = 22; // dBm (SX1262 max)

    int state = radio.begin(beacon::FREQ, beacon::BW, beacon::SF, beacon::CR,
                            beacon::SYNC, TX_POWER, beacon::PREAMBLE, 0.0, false);
    if (state != RADIOLIB_ERR_NONE) beacon::halt("begin", state, PIN_LED);

    state = radio.setDio2AsRfSwitch(true);
    if (state != RADIOLIB_ERR_NONE) beacon::halt("setDio2AsRfSwitch", state, PIN_LED);

    Serial.println(F("Beaconing every 5 s..."));
}

void loop() {
    String msg = "Radio Board beacon #" + String(counter++);

    Serial.print(F("[TX] "));
    Serial.println(msg);
    digitalWrite(PIN_LED, HIGH);

    int state = radio.transmit(msg);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("  TX failed, code "));
        Serial.println(state);
    }

    digitalWrite(PIN_LED, LOW);
    delay(5000);
}
