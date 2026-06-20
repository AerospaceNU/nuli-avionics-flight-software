/*
  RadioBoardSpiTest -- minimal SPI bring-up for the NULI Radio Board (RADIOGAGA).

  Goal: prove the SAMD51 can talk to the Semtech SX1262 over SPI by reading a
  batch of registers and comparing them against their documented power-on-reset
  defaults. The SX1262 has no real WHO_AM_I, so matching several known-default
  registers (sync word, CRC init/poly, whitening) is the strongest cheap proof
  that NSS/SCK/MOSI/MISO and the BUSY handshake are all working.

  Self-contained: only the Arduino SPI library. The global `SPI` object is
  mapped to SERCOM0 by the custom radio_board variant.

  Pin names come from the radio_board variant:
     PIN_RADIO_NSS  (PA04)  PIN_RADIO_BUSY (PA08)
     PIN_RADIO_DIO1 (PA09)  PIN_RADIO_RESET(PB11)
  SPI SCK/MISO/MOSI = PA05/PA06/PA07 on SERCOM0.
*/

#include <Arduino.h>
#include <SPI.h>

// ---- SX126x SPI opcode ---------------------------------------------------
static constexpr uint8_t SX126X_CMD_READ_REGISTER = 0x1D;

// Registers with documented power-on-reset defaults (SX1262 datasheet rev 2.1,
// Table 13-x). `expected` < 0 means "value varies, just display it".
struct RegCheck {
    uint16_t    addr;
    int16_t     expected;
    const char* name;
};

static const RegCheck kRegs[] = {
    // Asserted: canonical POR defaults that drivers rely on.
    { 0x06BC,  0x1D, "CRC init MSB"       },
    { 0x06BD,  0x0F, "CRC init LSB"       }, //  -> CRC init     = 0x1D0F
    { 0x06BE,  0x10, "CRC polynomial MSB" },
    { 0x06BF,  0x21, "CRC polynomial LSB" }, //  -> CRC poly     = 0x1021 (CCITT)
    { 0x0740,  0x14, "LoRa sync word MSB" },
    { 0x0741,  0x24, "LoRa sync word LSB" }, //  -> LoRa sync    = 0x1424 (private)
    // Informational only (no universally documented POR default / live data).
    { 0x06B8,  -1,   "Whitening init MSB" },
    { 0x06B9,  -1,   "Whitening init LSB" },
    { 0x0819,  -1,   "Random number byte" }, // RNG, changes each read
};

// SX1262 supports up to 16 MHz; keep it gentle for first bring-up.
static SPISettings radioSpi(2000000, MSBFIRST, SPI_MODE0);

// SX126x asserts BUSY while processing -- wait for it to drop. Returns false
// on timeout.
static bool waitBusy(uint32_t timeoutMs = 100) {
    uint32_t start = millis();
    while (digitalRead(PIN_RADIO_BUSY) == HIGH) {
        if (millis() - start > timeoutMs) return false;
    }
    return true;
}

static void radioReset() {
    pinMode(PIN_RADIO_RESET, OUTPUT);
    digitalWrite(PIN_RADIO_RESET, LOW);   // active-low reset
    delay(2);
    digitalWrite(PIN_RADIO_RESET, HIGH);
    delay(5);                             // datasheet: ~3.5ms POR
}

static uint8_t radioReadRegister(uint16_t addr) {
    waitBusy();
    SPI.beginTransaction(radioSpi);
    digitalWrite(PIN_RADIO_NSS, LOW);
    SPI.transfer(SX126X_CMD_READ_REGISTER);
    SPI.transfer(static_cast<uint8_t>(addr >> 8));
    SPI.transfer(static_cast<uint8_t>(addr & 0xFF));
    SPI.transfer(0x00);                   // NOP: status byte (discarded)
    uint8_t value = SPI.transfer(0x00);
    digitalWrite(PIN_RADIO_NSS, HIGH);
    SPI.endTransaction();
    return value;
}

static void print2Hex(uint8_t v) {
    if (v < 0x10) Serial.print('0');
    Serial.print(v, HEX);
}

// Read every register in the table, compare to its expected default, and
// report. Returns true if all checked registers matched.
static bool dumpRegisters() {
    bool allOk = true;
    for (const RegCheck& r : kRegs) {
        uint8_t val = radioReadRegister(r.addr);
        Serial.print(F("  0x"));
        print2Hex(static_cast<uint8_t>(r.addr >> 8));
        print2Hex(static_cast<uint8_t>(r.addr & 0xFF));
        Serial.print(F(" = 0x"));
        print2Hex(val);
        Serial.print(F("  "));
        Serial.print(r.name);
        if (r.expected < 0) {
            Serial.println(F("  (info, not checked)"));
        } else if (val == r.expected) {
            Serial.println(F("  OK"));
        } else {
            Serial.print(F("  MISMATCH (expected 0x"));
            print2Hex(static_cast<uint8_t>(r.expected));
            Serial.println(F(")"));
            allOk = false;
        }
    }
    return allOk;
}

void setup() {
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    pinMode(PIN_RADIO_NSS, OUTPUT);
    digitalWrite(PIN_RADIO_NSS, HIGH);    // deselect
    pinMode(PIN_RADIO_BUSY, INPUT);

    Serial.begin(115200);
    while (!Serial) { /* block until the USB serial monitor connects */ }

    Serial.println();
    Serial.println(F("=== NULI Radio Board SX1262 SPI bring-up ==="));

    SPI.begin();
    radioReset();
}

void loop() {
    Serial.println();
    bool busyOk = waitBusy(200);
    Serial.print(F("BUSY low after reset: "));
    Serial.println(busyOk ? F("yes") : F("NO (check power/RESET/BUSY wiring)"));

    bool allOk = dumpRegisters();

    Serial.print(F("--> "));
    Serial.println(allOk ? F("SPI OK: all register defaults match")
                         : F("FAIL: a register default did not match"));

    digitalWrite(PIN_LED, (busyOk && allOk) ? HIGH : LOW);
    delay(2000);
}
