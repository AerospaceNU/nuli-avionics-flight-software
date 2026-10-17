/** @brief EBYTE E22-900M30S range test: receiver, acks every message. */
#include <Arduino.h>
#include <RadioLib.h>

// Pins match Riley's test rig
#define RFM95_CS    8

#define E22_CS      10
#define E22_DIO1    13   // IRQ
#define E22_RST     6
#define E22_BUSY    9
#define E22_RXEN    14
#define E22_TXEN    15

SX1262 e22 = new Module(E22_CS, E22_DIO1, E22_RST, E22_BUSY);

String prevMsg;

volatile bool e22Received = false;
volatile bool e22Sent     = false;

void e22RxDone()   { e22Received = true; }
void e22TxDone()   { e22Sent = true; }

void setup() {
  Serial.begin(9600);
  delay(2000);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Initializing radio...");

  SPI.begin();
  SPI.begin();
  pinMode(RFM95_CS, OUTPUT);
  digitalWrite(RFM95_CS, HIGH);

  int state = e22.begin(915.0, 125, 12);
  e22.setOutputPower(20);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("E22 (SX1262) OK");
  } else {
    Serial.print("E22 fail, code "); 
    Serial.println(state);
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
    }
  }


  e22.setRfSwitchPins(E22_RXEN, E22_TXEN);

  e22.setDio1Action(e22RxDone);

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
      delay(500);
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
