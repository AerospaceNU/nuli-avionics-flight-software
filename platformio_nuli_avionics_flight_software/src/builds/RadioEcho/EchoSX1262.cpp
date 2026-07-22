/*
  EchoSX1262.cpp -- LoRa echo responder on the NULI Radio Board.

  Listens on the shared on-air params. Whenever a packet arrives, it echoes the
  message straight back over the air with the board's millis() timestamp
  appended, e.g. a received "hello" becomes "hello @12345ms". Pair with any
  matching node (RadioChatFeatherM0, BeaconRx, etc.) to see the reply.

  Hardware: SAMD51J19A (Cortex-M4) + Semtech SX1262 on the custom radio_board
  variant (SPI on SERCOM0).

  Board specifics (same as the chat / beacon builds):
    - TCXO (Y2) is powered from the 3V3 rail, NOT from the SX1262 DIO3, so we
      pass tcxoVoltage = 0 to stop RadioLib driving DIO3 as a TCXO supply.
    - DIO2 drives the antenna TX/RX switch -> setDio2AsRfSwitch(true).

  On-air params match RadioChatCommon.h / RadioBeaconCommon.h so existing nodes
  interoperate.

  Env: [env:RadioEchoSX1262]
*/

#include <Arduino.h>
#include <RadioLib.h>

// ---- on-air parameters (must match the other node) ----------------------
namespace {
constexpr float    FREQ     = 915.0;  // MHz (US ISM / board is 915 band)
constexpr float    BW       = 125.0;  // kHz
constexpr uint8_t  SF       = 9;      // spreading factor
constexpr uint8_t  CR       = 7;      // coding rate 4/7
constexpr uint8_t  SYNC     = 0x12;   // private network sync word
constexpr uint16_t PREAMBLE = 8;      // symbols
constexpr int8_t   TX_POWER = 22;     // dBm (SX1262 max)
}

// CS/NSS, DIO1 (IRQ), RESET, BUSY -- names come from the radio_board variant.
// Cast to uint32_t so PIN_RADIO_NSS (== 0u, a null-pointer constant) doesn't
// make the Module(RadioLibHal*, ...) overload ambiguous.
static SX1262 radio = new Module(static_cast<uint32_t>(PIN_RADIO_NSS),
                                 static_cast<uint32_t>(PIN_RADIO_DIO1),
                                 static_cast<uint32_t>(PIN_RADIO_RESET),
                                 static_cast<uint32_t>(PIN_RADIO_BUSY));

// ---- receive interrupt flag ---------------------------------------------
static volatile bool rxFlag = false;
static void onRx() { rxFlag = true; }

// Fatal error: report and blink the LED forever.
static void halt(const char* what, int code) {
    Serial.print(F("[FATAL] "));
    Serial.print(what);
    Serial.print(F(" failed, code "));
    Serial.println(code);
    pinMode(PIN_LED, OUTPUT);
    while (true) {
        digitalWrite(PIN_LED, HIGH); delay(100);
        digitalWrite(PIN_LED, LOW);  delay(100);
    }
}

void setup() {
    pinMode(PIN_LED, OUTPUT);

    Serial.begin(115200);
    // Don't block forever -- this node should echo even with no monitor open.
    uint32_t t0 = millis();
    while (!Serial && (millis() - t0 < 2000)) { /* brief wait for the monitor */ }

    Serial.println();
    Serial.println(F("=== Radio Echo :: SX1262 (NULI Radio Board) ==="));

    // tcxoVoltage = 0.0 -> external always-on TCXO; useRegulatorLDO = false -> DC-DC.
    int state = radio.begin(FREQ, BW, SF, CR, SYNC, TX_POWER, PREAMBLE, 0.0, false);
    if (state != RADIOLIB_ERR_NONE) halt("begin", state);

    state = radio.setDio2AsRfSwitch(true);
    if (state != RADIOLIB_ERR_NONE) halt("setDio2AsRfSwitch", state);

    radio.setPacketReceivedAction(onRx);
    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) halt("startReceive", state);

    Serial.println(F("Listening. Every received packet is echoed back with a millis timestamp."));
}

void loop() {
    if (!rxFlag) return;
    rxFlag = false;

    String msg;
    int state = radio.readData(msg);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[RX error, code "));
        Serial.print(state);
        Serial.println(F("]"));
        radio.startReceive();
        return;
    }

    Serial.print(F("[RX] "));
    Serial.print(msg);
    Serial.print(F("   (RSSI "));
    Serial.print(radio.getRSSI());
    Serial.print(F(" dBm, SNR "));
    Serial.print(radio.getSNR());
    Serial.println(F(" dB)"));

    // Echo the message back with our current uptime appended.
    String reply = msg + " @" + String(millis()) + "ms";

    Serial.print(F("[TX] "));
    Serial.println(reply);
    digitalWrite(PIN_LED, HIGH);

    state = radio.transmit(reply);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("  TX failed, code "));
        Serial.println(state);
    }

    digitalWrite(PIN_LED, LOW);

    rxFlag = false;           // ignore the self-trigger from our own TX
    radio.startReceive();     // back to listening
}
