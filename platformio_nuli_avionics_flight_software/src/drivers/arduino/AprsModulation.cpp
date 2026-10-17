#include "AprsModulation.h"
#include <Arduino.h>

// Much of this file was taken from: https://github.com/handiko/Arduino-APRS

/*
 *  Copyright (C) 2018 - Handiko Gesang - www.github.com/handiko
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _1200   1
#define _2400   0
#define APRS_FLAG       0x7e
#define APRS_CTRL_ID    0x03
#define APRS_PID        0xf0
#define APRS_DT_STATUS  '>'

uint8_t OUT_PIN = 0;
bool nada = _2400;

// tc1200/tc2400: half-periods of the 1200/2400 Hz tones; baud_adj trims both together
const float baud_adj = 0.975;
const float adj_1200 = 1.0 * baud_adj;
const float adj_2400 = 1.0 * baud_adj;
unsigned int tc1200 = (unsigned int) (0.5 * adj_1200 * 1000000.0 / 1200.0);
unsigned int tc2400 = (unsigned int) (0.5 * adj_2400 * 1000000.0 / 2400.0);

const char* myCallsign = "MYCALL";
char mySsid = 1;

const char* dest = "APRS";

const char* digi = "WIDE2";
char digiSsid = 1;

char bitStuff = 0;
unsigned short crc = 0xffff;


void setNada1200();

void setNada2400();

void setNada(bool nada);

void sendCharNRZI(unsigned char in_byte, bool enBitStuff);

void sendStringLen(const char* in_string, int len);

void calcCrc(bool in_bit);

void sendCrc();

void sendPacket(const char* myStatus);

void sendFlag(unsigned char flag_len);

void sendHeader();

void sendPayload(const char* myStatus);

void setNada1200() {
    digitalWrite(OUT_PIN, HIGH);
    delayMicroseconds(tc1200);
    digitalWrite(OUT_PIN, LOW);
    delayMicroseconds(tc1200);
}

void setNada2400() {
    digitalWrite(OUT_PIN, HIGH);
    delayMicroseconds(tc2400);
    digitalWrite(OUT_PIN, LOW);
    delayMicroseconds(tc2400);

    digitalWrite(OUT_PIN, HIGH);
    delayMicroseconds(tc2400);
    digitalWrite(OUT_PIN, LOW);
    delayMicroseconds(tc2400);
}

void setNada(bool nadaA) {
    if (nadaA) {
        setNada1200();
    } else {
        setNada2400();
    }
}

// CRC-16 CCITT (poly 0x1021, init 0xFFFF) for the AX.25 FCS
void calcCrc(bool in_bit) {
    unsigned short xor_in;

    xor_in = crc ^ in_bit;
    crc >>= 1;

    if (xor_in & 0x01)
        crc ^= 0x8408;
}

void sendCrc() {
    unsigned char crc_lo = crc ^ 0xff;
    unsigned char crc_hi = (crc >> 8) ^ 0xff;

    sendCharNRZI(crc_lo, HIGH);
    sendCharNRZI(crc_hi, HIGH);
}

void sendHeader() {
    char temp;

    // AX.25 header: DEST/SOURCE/DIGI (6-char callsign + SSID, shifted left 1, DIGI SSID +1), CTRL 0x03, PID 0xf0

    /********* DEST ***********/
    temp = strlen(dest);
    for (int j = 0; j < temp; j++)
        sendCharNRZI(dest[j] << 1, HIGH);
    if (temp < 6) {
        for (int j = 0; j < (6 - temp); j++)
            sendCharNRZI(' ' << 1, HIGH);
    }
    sendCharNRZI('0' << 1, HIGH);


    /********* SOURCE *********/
    temp = strlen(myCallsign);
    for (int j = 0; j < temp; j++)
        sendCharNRZI(myCallsign[j] << 1, HIGH);
    if (temp < 6) {
        for (int j = 0; j < (6 - temp); j++)
            sendCharNRZI(' ' << 1, HIGH);
    }
    sendCharNRZI((mySsid + '0') << 1, HIGH);


    /********* DIGI ***********/
    temp = strlen(digi);
    for (int j = 0; j < temp; j++)
        sendCharNRZI(digi[j] << 1, HIGH);
    if (temp < 6) {
        for (int j = 0; j < (6 - temp); j++)
            sendCharNRZI(' ' << 1, HIGH);
    }
    sendCharNRZI(((digiSsid + '0') << 1) + 1, HIGH);

    /***** CTRL FLD & PID *****/
    sendCharNRZI(APRS_CTRL_ID, HIGH);
    sendCharNRZI(APRS_PID, HIGH);
}

void sendPayload(const char* myStatus) {
    // Status payload: '>' then free-form ASCII text, not shifted

    sendCharNRZI(APRS_DT_STATUS, HIGH);
    sendStringLen(myStatus, strlen(myStatus));
}

// Sends one byte LSB first, NRZI: 1 = no tone change, 0 = tone change
void sendCharNRZI(unsigned char in_byte, bool enBitStuff) {
    bool bits;

    for (int i = 0; i < 8; i++) {
        bits = in_byte & 0x01;

        calcCrc(bits);

        if (bits) {
            setNada(nada);
            bitStuff++;

            if ((enBitStuff) && (bitStuff == 5)) {
                nada ^= 1;
                setNada(nada);

                bitStuff = 0;
            }
        } else {
            nada ^= 1;
            setNada(nada);

            bitStuff = 0;
        }

        in_byte >>= 1;
    }
}

void sendStringLen(const char* in_string, int len) {
    for (int j = 0; j < len; j++)
        sendCharNRZI(in_string[j], HIGH);
}

void sendFlag(unsigned char flag_len) {
    for (int j = 0; j < flag_len; j++)
        sendCharNRZI(APRS_FLAG, LOW);
}

void sendPacket(const char* myStatus) {

    sendFlag(60);
    crc = 0xffff;
    sendHeader();
    sendPayload(myStatus);
    sendCrc();
    sendFlag(3);
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



AprsModulation::AprsModulation(uint8_t transmitPin, const char* callsign) {
    uint32_t i = 0;
    for (; i < sizeof(m_callsign) - 1 && callsign[i] != '\0'; i++) {
        m_callsign[i] = callsign[i];
    }
    m_callsign[i] = '\0';
    myCallsign = m_callsign;

    m_transmitPin = transmitPin;
    OUT_PIN = m_transmitPin;
}

void AprsModulation::setup() {
    pinMode(m_transmitPin, OUTPUT);
}

void AprsModulation::transmit(uint8_t* data, uint32_t length) {
    sendPacket((char*) data);
    digitalWrite(OUT_PIN, LOW);
}

const char* AprsModulation::getCallsign() {
    return m_callsign;
}