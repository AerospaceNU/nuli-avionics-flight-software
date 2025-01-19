#include <SPI.h>

#include <S25FL512.h>

#define CS_PIN 12

void printBinaryLn(byte inByte) {
    for (int b = 7; b >= 0; b--)
        Serial.print(bitRead(inByte, b));
    Serial.println();
}

void printBinary(byte inByte) {
    for (int b = 7; b >= 0; b--)
        Serial.print(bitRead(inByte, b));
    Serial.println();
}


void loop() {
//    digitalWrite(A3, HIGH);
//    delay(1000);
//    digitalWrite(A3, LOW);
//    delay(1000);

}

S25FL512 flash(12);

void readPrint2(uint32_t address, uint32_t length) {
    uint8_t buff[1000];
    flash.read(address, buff, length);
//    readData(address, buff, length);
    for (uint32_t i = 0; i < length; i++) {
        printBinaryLn(buff[i]);
    }
}


void setup() {
//    pinMode(A3, OUTPUT);

    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);

    Serial.begin(9600);
    while (!Serial);
    SPI.begin();

    uint8_t magicNum = 0x73;

    flash.write(0, magicNum);
    flash.write(S25FL512::getSectorSize() * 3 + 1, magicNum);
    flash.write(S25FL512::getMemorySize() - 1, magicNum);

    uint8_t buff[8000];
    for(uint32_t i = 0; i < S25FL512::getMemorySize(); i += sizeof(buff)) {
        flash.read(i, buff, sizeof(buff));

        for(uint32_t j = 0; j < sizeof(buff); j++) {
            uint8_t a = buff[j];
            if(a == magicNum) {
                Serial.print("Found at: ");
                Serial.println(i + j);
            }
            if((i + j) % S25FL512::getSectorSize() == 0) {
                Serial.print("Sector: ");
                Serial.println((i + j) / S25FL512::getSectorSize());
            }
        }
    }

    Serial.println("Done, erasing");
    Serial.println(flash.read(0));
    flash.eraseAll(true);
    Serial.println(flash.read(0));

//    for(uint32_t i = 0; i < S25FL512::getMemorySize(); i++) {
//        uint8_t a = flash.read(i);
//        if(a == magicNum) {
//            Serial.print("Found at: ");
//            Serial.println(i);
//        }
//        if(i % S25FL512::getSectorSize() == 0) {
//            Serial.print("Sector: ");
//            Serial.println(i / S25FL512::getSectorSize());
//        }
//    }

//    Serial.println("Done");


////    Serial.println(readByte(10));
//
//    Serial.println("Reading: ");
//    readPrint2(3, 3);
//
//
//    flash.eraseAll(true);
//    flash.eraseSector(0, true);
////    uint8_t data[3] = {0b01010101, 0b00000000, 0b11110000};
////    flash.write(3, data, 3, true);
//
//
//    Serial.println("Reading: ");
//    readPrint2(3, 3);
}
