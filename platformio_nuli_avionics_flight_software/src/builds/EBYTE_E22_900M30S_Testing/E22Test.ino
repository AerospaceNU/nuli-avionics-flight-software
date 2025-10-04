/*
This is a test script to test the EBYTE Radio Module with the Adafruit Feather M0
It Pings back and forth between the onboard RFM95 and the EBYTE E22-900M30S
*/

#include <RadioLib.h>

// -------------------- Feather M0 onboard RFM95 --------------------
#define RFM95_CS    8
#define RFM95_DIO0  3
#define RFM95_RST   4

// -------------------- External E22-900M30S (SX1262) --------------------
#define E22_CS      10
#define E22_DIO1    13   // SX1262 uses DIO1 as IRQ
#define E22_RST     6
#define E22_BUSY    9
#define E22_RXEN    14  // connect to RXEN pin on E22
#define E22_TXEN    15  // connect to TXEN pin on E22

// Create radio instances
RFM95  rfm = new Module(RFM95_CS, RFM95_DIO0, RFM95_RST);
SX1262 e22 = new Module(E22_CS, E22_DIO1, E22_RST, E22_BUSY);

// Flags
volatile bool rfmReceived = false;
volatile bool e22Received = false;
volatile bool rfmSent     = false;
volatile bool e22Sent     = false;

// ISR callbacks
void rfmRxDone()   { rfmReceived = true; }
void e22RxDone()   { e22Received = true; }
void rfmTxDone()   { rfmSent = true; }
void e22TxDone()   { e22Sent = true; }

void setup() {
  Serial.begin(9600);
  //wait for Serial
  delay(2000);

  Serial.println(F("Initializing radios..."));

  // Init RFM95
  int state = rfm.begin(915.0, 125, 12);
  rfm.setOutputPower(20);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("RFM95 OK"));
  } else {
    Serial.print(F("RFM95 fail, code ")); Serial.println(state);
    while (true);
  }

  // Init E22 (SX1262)
  state = e22.begin(915.0, 125, 12);
  e22.setOutputPower(20);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("E22 (SX1262) OK"));
  } else {
    Serial.print(F("E22 fail, code ")); Serial.println(state);
    while (true);
  }


  // Set RF switch control pins for E22
  e22.setRfSwitchPins(E22_RXEN, E22_TXEN);

  // Attach ISRs
  rfm.setDio0Action(rfmRxDone, RISING);
  e22.setDio1Action(e22RxDone);  // SX1262 uses DIO1
  // (RadioLib automatically uses BUSY line for SX126x)

  // Start in receive mode
  rfm.startReceive();
  e22.startReceive();

  // Initiator: onboard RFM95 sends first ping
  Serial.println(F("[RFM95] Initiating ping..."));
  rfm.setDio0Action(rfmTxDone, RISING);
  rfm.startTransmit("Ping from RFM95");
}

void loop() {
  // -------------------- RFM95 received something --------------------
  if (rfmReceived) {
    rfmReceived = false;
    String msg;
    int state = rfm.readData(msg);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.print(F("[RFM95] Got: ")); Serial.println(msg);
      delay(1000);  // wait 1 second before reply
      rfm.setDio0Action(rfmTxDone, RISING);
      rfm.startTransmit("Reply from RFM95");
      Serial.println(F("[RFM95] Replying..."));
    }
  }

  // -------------------- E22 received something --------------------
  if (e22Received) {
    e22Received = false;
    String msg;
    int state = e22.readData(msg);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.print(F("[E22] Got: ")); Serial.println(msg);
      delay(1000);  // wait 1 second before reply
      e22.setDio1Action(e22TxDone);
      e22.startTransmit("Reply from E22");
      Serial.println(F("[E22] Replying..."));
    }
  }

  // -------------------- TX done events --------------------
  if (rfmSent) {
    rfmSent = false;
    Serial.println(F("[RFM95] TX done, listening..."));
    rfm.setDio0Action(rfmRxDone, RISING);
    rfm.startReceive();
  }
  if (e22Sent) {
    e22Sent = false;
    Serial.println(F("[E22] TX done, listening..."));
    e22.setDio1Action(e22RxDone);
    e22.startReceive();
  }
}
