/** @brief EBYTE E22-900M30S range test: transmitter, sends a counter and prints the ack's RSSI/SNR. */

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

int transmitCount = 0;
uint32_t timeSinceReceived = 0;

volatile bool e22Received = false;
volatile bool e22Sent     = false;


void e22RxDone()   { e22Received = true; }
void e22TxDone()   { e22Sent = true; }

void setup() {
  Serial.begin(9600);
  delay(2000);

  Serial.println(F("Initializing radio..."));
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  SPI.begin();
  pinMode(RFM95_CS, OUTPUT);
  digitalWrite(RFM95_CS, HIGH);


  int state = e22.begin(915.0, 125, 12);
  e22.setOutputPower(20);
  if (state == RADIOLIB_ERR_NONE) {
  } else {
    Serial.print(F("E22 fail, code ")); Serial.println(state);
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
    }
  }

  e22.setRfSwitchPins(E22_RXEN, E22_TXEN);

  e22.setDio1Action(e22RxDone);

  // Fake a receive so loop() sends the first message
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
      float rssi = e22.getRSSI();
      float snr  = e22.getSNR();
      Serial.print("RSSI: ");
      Serial.print(rssi);
      Serial.print(" dBm | SNR: ");
      Serial.print(snr);
      Serial.println(" dB");

      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
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
