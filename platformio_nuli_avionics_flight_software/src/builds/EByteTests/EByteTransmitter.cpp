/**
This is a script for the radio range testing of the EBYTE E22-900M30S
This is the Code for the Receiver
*/

#include <Arduino.h>
#include <RadioLib.h>
#include <string.h>
#include <cstring>

// External E22-900M30S (SX1262)
// Redefine these pins as necessary. It is correct for the one I put together - Riley
#define RFM95_CS    8

#define E22_CS      10   // NSS pin
#define E22_DIO1    13   // SX1262 uses DIO1 as IRQ
#define E22_RST     6
#define E22_BUSY    9
#define E22_RXEN    14
#define E22_TXEN    15

// Create radio instance
SX1262 e22 = new Module(E22_CS, E22_DIO1, E22_RST, E22_BUSY);

int transmitCount = 0;
uint32_t timeSinceReceived = 0;

// Flags
volatile bool e22Received = false;
volatile bool e22Sent     = false;


// ISR callbacks
void e22RxDone()   { e22Received = true; }
void e22TxDone()   { e22Sent = true; }

void setup() {
  Serial.begin(9600);
  //delay for serial connection
  delay(2000);

  Serial.println(F("Initializing radio..."));
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  SPI.begin();
  pinMode(RFM95_CS, OUTPUT);
  digitalWrite(RFM95_CS, HIGH);


  // Init E22 (SX1262)
  int state = e22.begin(915.0, 125, 12);
  e22.setOutputPower(20);
  if (state == RADIOLIB_ERR_NONE) {
  } else {
    Serial.print(F("E22 fail, code ")); Serial.println(state);
    //if it fails to start, blink the led light
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
    }
  }

  // Set RF switch control pins for E22
  e22.setRfSwitchPins(E22_RXEN, E22_TXEN);

  // Attach ISRs
  e22.setDio1Action(e22RxDone);  // SX1262 uses DIO1
  // (RadioLib automatically uses BUSY line for SX126x)

  // Start in receive mode
  e22Received = true;
  timeSinceReceived = millis();

}

void loop() {
  String ack;
 
  // -------------------- E22 received something --------------------
  if (e22Received) {
    e22Received = false;
    int state = e22.readData(ack);
    if (state == RADIOLIB_ERR_NONE) {
      float rssi = e22.getRSSI();   // in dBm
      float snr  = e22.getSNR();    // in dB (may be negative)
      Serial.print("RSSI: ");
      Serial.print(rssi);
      Serial.print(" dBm | SNR: ");
      Serial.print(snr);
      Serial.println(" dB");

      digitalWrite(LED_BUILTIN, LOW);
      delay(500);  // wait 1/2 second before reply
      transmitCount += 1;
      String convert = String(transmitCount);
      e22.startTransmit(convert);
    }
    else {
      Serial.println("Radio Error Detected: " + state);
    }
    Serial.println("Transmitted " + String(transmitCount) + " times");
    e22.setDio1Action(e22TxDone);
  }

  if ((millis() - timeSinceReceived) / 1000 > 5) {
    Serial.println("Timed out, resending Message: " + String(transmitCount));
    digitalWrite(LED_BUILTIN, HIGH);
    String convert = String(transmitCount);
    e22.startTransmit(convert);
    timeSinceReceived = millis();
    e22.setDio1Action(e22TxDone);
  }


  // -------------------- TX done events --------------------
  if (e22Sent) {
    e22Sent = false;
    timeSinceReceived = millis();
    e22.setDio1Action(e22RxDone);
    e22.startReceive();
  }
}
