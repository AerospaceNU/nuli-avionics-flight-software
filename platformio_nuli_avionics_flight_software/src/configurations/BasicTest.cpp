


// include the library
#include <RadioLib.h>

#define INITIATING_NODE
SX1276 radio = new Module(8, 3, 4);  // (CS, INT, RST)


// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;
// flag to indicate transmission or reception state
bool transmitFlag = false;
// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

void setFlag() {
    operationDone = true; // we sent or received  packet, set the flag
}

void setup() {
    Serial.begin(9600);
//    while (!Serial);

    // initialize SX1278 with default settings
    Serial.print(F("[SX1278] Initializing ... "));
//    int state = radio.begin();
    int state = radio.begin(905.0, 125.0, 12);
    radio.setOutputPower(20);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true) { delay(10); }
    }

    // set the function that will be called
    // when new packet is received
    radio.setDio0Action(setFlag, RISING);

#if defined(INITIATING_NODE)
    // send the first packet on this node
    Serial.print(F("[SX1278] Sending first packet ... "));
    transmissionState = radio.startTransmit("Hello World!");
    transmitFlag = true;
#else
    // start listening for LoRa packets on this node
    Serial.print(F("[SX1278] Starting to listen ... "));
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true) { delay(10); }
    }
#endif
}

void loop() {
    // check if the previous operation finished
    if (operationDone) {
        // reset flag
        operationDone = false;

//        if (transmitFlag) {
        // the previous operation was transmission, listen for response
        // print the result
        if (transmissionState == RADIOLIB_ERR_NONE) {
            // packet was successfully sent
            Serial.println(F("transmission finished!"));

        } else {
            Serial.print(F("failed, code "));
            Serial.println(transmissionState);

        }

        // listen for response
//            radio.startReceive();
//            transmitFlag = false;
//
//        } else {
//            // the previous operation was reception
//            // print data and send another packet
//            String str;
//            int state = radio.readData(str);
//
//            if (state == RADIOLIB_ERR_NONE) {
//                // packet was successfully received
//                Serial.println(F("[SX1278] Received packet!"));
//
//                // print data of the packet
//                Serial.print(F("[SX1278] Data:\t\t"));
//                Serial.println(str);
//
//                // print RSSI (Received Signal Strength Indicator)
//                Serial.print(F("[SX1278] RSSI:\t\t"));
//                Serial.print(radio.getRSSI());
//                Serial.println(F(" dBm"));
//
//                // print SNR (Signal-to-Noise Ratio)
//                Serial.print(F("[SX1278] SNR:\t\t"));
//                Serial.print(radio.getSNR());
//                Serial.println(F(" dB"));
//
//            }
//
//            // wait a second before transmitting again
//            delay(1000);

        // send another one
//        delay(1000);
        radio.startReceive();
        while (!Serial.available()) {
            String str;
            int state = radio.readData(str);
            if (operationDone) {
                operationDone = false;
                if (state == RADIOLIB_ERR_NONE) {
                    // packet was successfully received
                    Serial.println(F("[SX1278] Received packet!"));

                    // print data of the packet
                    Serial.print(F("[SX1278] Data:\t\t"));
                    Serial.println(str);

                    // print RSSI (Received Signal Strength Indicator)
                    Serial.print(F("[SX1278] RSSI:\t\t"));
                    Serial.print(radio.getRSSI());
                    Serial.println(F(" dBm"));

                    // print SNR (Signal-to-Noise Ratio)
                    Serial.print(F("[SX1278] SNR:\t\t"));
                    Serial.print(radio.getSNR());
                    Serial.println(F(" dB"));

                }
            }
        }
        String input = Serial.readString();  // Read entire input as a string

        Serial.print(F("[SX1278] Sending another packet ... "));
        transmissionState = radio.startTransmit(input.c_str());
        transmitFlag = true;
//        }
    }
}



//#include <Arduino.h>
//#include <RadioLib.h>
//
//SX1276 radio = new Module(8, 3, 4);  // (CS, INT, RST)
//
//
//void setup() {
//    Serial.begin(9600);
//    Serial1.begin(9600);
//    while (!Serial);
////    pinMode(LED_BUILTIN, OUTPUT);
//}
//
//
//void loop() {
//
//
//
////    if(Serial1.available())
////    Serial.print((char) Serial1.read());
//
////    digitalWrite(LED_BUILTIN, HIGH);
////    tone(0, 4000);
////    delay(1000);
////    digitalWrite(LED_BUILTIN, LOW);
////    noTone(0);
////    delay(1000);
//}