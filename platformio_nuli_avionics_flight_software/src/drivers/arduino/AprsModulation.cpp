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

// Defines the Square Wave Output Pin
#define _1200   1
#define _2400   0
#define APRS_FLAG       0x7e
#define APRS_CTRL_ID    0x03
#define APRS_PID        0xf0
#define APRS_DT_STATUS  '>'

uint8_t OUT_PIN = 0;
bool nada = _2400;

/*
 * SQUARE WAVE SIGNAL GENERATION
 *
 * baud_adj lets you to adjust or fine tune overall baud rate
 * by simultaneously adjust the 1200 Hz and 2400 Hz tone,
 * so that both tone would scales synchronously.
 * adj_1200 determined the 1200 hz tone adjustment.
 * tc1200 is the half of the 1200 Hz signal periods.
 *
 *      -------------------------                           -------
 *     |                         |                         |
 *     |                         |                         |
 *     |                         |                         |
 * ----                           -------------------------
 *
 *     |<------ tc1200 --------->|<------ tc1200 --------->|
 *
 * adj_2400 determined the 2400 hz tone adjustment.
 * tc2400 is the half of the 2400 Hz signal periods.
 *
 *      ------------              ------------              -------
 *     |            |            |            |            |
 *     |            |            |            |            |
 *     |            |            |            |            |
 * ----              ------------              ------------
 *
 *     |<--tc2400-->|<--tc2400-->|<--tc2400-->|<--tc2400-->|
 *
 */
const float baud_adj = 0.975;
const float adj_1200 = 1.0 * baud_adj;
const float adj_2400 = 1.0 * baud_adj;
unsigned int tc1200 = (unsigned int) (0.5 * adj_1200 * 1000000.0 / 1200.0);
unsigned int tc2400 = (unsigned int) (0.5 * adj_2400 * 1000000.0 / 2400.0);

/*
 * This strings will be used to generate AFSK signals, over and over again.
 */
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

/*
 * This function will calculate CRC-16 CCITT for the FCS (Frame Check Sequence)
 * as required for the HDLC frame validity check.
 *
 * Using 0x1021 as polynomial generator. The CRC registers are initialized with
 * 0xFFFF
 */
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

    /*
     * APRS AX.25 Header
     * ........................................................
     * |   DEST   |  SOURCE  |   DIGI   | CTRL FLD |    PID   |
     * --------------------------------------------------------
     * |  7 bytes |  7 bytes |  7 bytes |   0x03   |   0xf0   |
     * --------------------------------------------------------
     *
     * DEST   : 6 byte "callsign" + 1 byte ssid
     * SOURCE : 6 byte your callsign + 1 byte ssid
     * DIGI   : 6 byte "digi callsign" + 1 byte ssid
     *
     * ALL DEST, SOURCE, & DIGI are left shifted 1 bit, ASCII format.
     * DIGI ssid is left shifted 1 bit + 1
     *
     * CTRL FLD is 0x03 and not shifted.
     * PID is 0xf0 and not shifted.
     */

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
    /*
     * APRS AX.25 Payloads
     * TYPE : STATUS
     * ..................................
     * |DATA TYPE |    STATUS TEXT      |
     * ----------------------------------
     * |  1 byte  |       N bytes       |
     * ----------------------------------
     * DATA TYPE  : > STATUS TEXT: Free form text
     * All the data are sent in the form of ASCII Text, not shifted.
     */

    sendCharNRZI(APRS_DT_STATUS, HIGH);
    sendStringLen(myStatus, strlen(myStatus));
}

/*
 * This function will send one byte input and convert it
 * into AFSK signal one bit at a time LSB first.
 *
 * The encode which used is NRZI (Non Return to Zero, Inverted)
 * bit 1 : transmitted as no change in tone
 * bit 0 : transmitted as change in tone
 */
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

/*
 * In this preliminary test, a packet is consists of FLAG(s) and PAYLOAD(s).
 * Standard APRS FLAG is 0x7e character sent over and over again as a packet
 * delimiter. In this example, 100 flags is used the preamble and 3 flags as
 * the postamble.
 */
void sendPacket(const char* myStatus) {

    /*
     * AX25 FRAME
     *
     * ........................................................
     * |  FLAG(s) |  HEADER  | PAYLOAD  | FCS(CRC) |  FLAG(s) |
     * --------------------------------------------------------
     * |  N bytes | 22 bytes |  N bytes | 2 bytes  |  N bytes |
     * --------------------------------------------------------
     *
     * FLAG(s)  : 0x7e
     * HEADER   : see header
     * PAYLOAD  : 1 byte data type + N byte info
     * FCS      : 2 bytes calculated from HEADER + PAYLOAD
     */

    sendFlag(60);      // was: 100
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