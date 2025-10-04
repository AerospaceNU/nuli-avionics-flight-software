/**
This is a script for the radio range testing of the EBYTE E22-900M30S
This is the Code for the Receiver
*/
#include <Arduino.h>
#include <RadioLib.h>

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

String prevMsg;

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
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Initializing radio...");

  SPI.begin();
  SPI.begin();
  pinMode(RFM95_CS, OUTPUT);
  digitalWrite(RFM95_CS, HIGH);

  // Init E22 (SX1262)
  int state = e22.begin(915.0, 125, 12);
  e22.setOutputPower(20);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("E22 (SX1262) OK");
  } else {
    Serial.print("E22 fail, code "); 
    Serial.println(state);
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
  e22.startReceive();

}

void loop() {
  String msg;
  
 
  // -------------------- E22 received something --------------------
  if (e22Received) {
    e22Received = false;
    int state = e22.readData(msg);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("Received: " + msg); 
      delay(500);  // wait 1/2 second before reply
      e22.setDio1Action(e22TxDone);
      if (msg == prevMsg) {
        Serial.println("Duplicate detected, resending Ack");
      }
      e22.startTransmit("Ack " + msg);
      Serial.println("Ack " + msg + " Sent");
      prevMsg = msg;
    }
    else {
      Serial.println("Radio Error Detected: " + state);
    }
  }


  // -------------------- TX done events --------------------
  if (e22Sent) {
    e22Sent = false;
    e22.setDio1Action(e22RxDone);
    e22.startReceive();
  }
}
