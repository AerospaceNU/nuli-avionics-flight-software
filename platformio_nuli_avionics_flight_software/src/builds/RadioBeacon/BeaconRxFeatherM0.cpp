/*
  BeaconRxFeatherM0.cpp -- receive-only LoRa node on an Adafruit Feather M0 RFM95.

  Listens continuously and prints every packet it receives (with RSSI / SNR).
  Pair with BeaconTxSX1262.cpp on the NULI Radio Board.

  Standard Feather M0 RFM95 wiring: CS = 8, RST = 4, DIO0 (IRQ) = 3.

  Env: [env:BeaconRxFeatherM0]
*/

#include "RadioBeaconCommon.h"

#define RFM95_CS    8
#define RFM95_DIO0  3   // IRQ
#define RFM95_RST   4

static SX1276 radio = new Module(RFM95_CS, RFM95_DIO0, RFM95_RST, RADIOLIB_NC);

static volatile bool rxFlag = false;
static void onRx() { rxFlag = true; }

void setup() {
    Serial.begin(115200);
    while (!Serial) { /* wait for the USB serial monitor */ }

    Serial.println();
    Serial.println(F("=== Beacon RX :: SX1276 (Feather M0 RFM95) ==="));

    // Power is irrelevant for a receiver; any valid value works.
    int state = radio.begin(beacon::FREQ, beacon::BW, beacon::SF, beacon::CR,
                            beacon::SYNC, 10, beacon::PREAMBLE);
    if (state != RADIOLIB_ERR_NONE) beacon::halt("begin", state, LED_BUILTIN);

    radio.setPacketReceivedAction(onRx);
    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) beacon::halt("startReceive", state, LED_BUILTIN);

    Serial.println(F("Listening..."));
}

void loop() {
    if (!rxFlag) return;
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
