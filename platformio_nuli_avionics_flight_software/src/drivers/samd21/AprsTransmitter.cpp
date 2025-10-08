    #include "AprsTransmitter.h"
#include <Arduino.h>

const uint8_t SIN_1200_HZ[] PROGMEM = {
        0x7F, 0x82, 0x86, 0x8A, 0x8E, 0x92, 0x96, 0x9A, 0x9E, 0xA2, 0xA6, 0xAA, 0xAD, 0xB1, 0xB5, 0xB8, 0xBC, 0xBF, 0xC3, 0xC6, 0xC9, 0xCC, 0xCF, 0xD2, 0xD5, 0xD8, 0xDB, 0xDE, 0xE0, 0xE3, 0xE5, 0xE8,
        0xEA, 0xEC, 0xEE, 0xF0, 0xF1, 0xF3, 0xF5, 0xF6, 0xF7, 0xF8, 0xFA, 0xFA, 0xFB, 0xFC, 0xFC, 0xFD, 0xFD, 0xFD, 0xFE, 0xFD, 0xFD, 0xFD, 0xFC, 0xFC, 0xFB, 0xFA, 0xFA, 0xF8, 0xF7, 0xF6, 0xF5, 0xF3,
        0xF1, 0xF0, 0xEE, 0xEC, 0xEA, 0xE8, 0xE5, 0xE3, 0xE0, 0xDE, 0xDB, 0xD8, 0xD5, 0xD2, 0xCF, 0xCC, 0xC9, 0xC6, 0xC3, 0xBF, 0xBC, 0xB8, 0xB5, 0xB1, 0xAD, 0xAA, 0xA6, 0xA2, 0x9E, 0x9A, 0x96, 0x92,
        0x8E, 0x8A, 0x86, 0x82, 0x7F, 0x7B, 0x77, 0x73, 0x6F, 0x6B, 0x67, 0x63, 0x5F, 0x5B, 0x57, 0x53, 0x50, 0x4C, 0x48, 0x45, 0x41, 0x3E, 0x3A, 0x37, 0x34, 0x31, 0x2E, 0x2B, 0x28, 0x25, 0x22, 0x1F,
        0x1D, 0x1A, 0x18, 0x15, 0x13, 0x11, 0x0F, 0x0D, 0x0C, 0x0A, 0x08, 0x07, 0x06, 0x05, 0x03, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x05,
        0x06, 0x07, 0x08, 0x0A, 0x0C, 0x0D, 0x0F, 0x11, 0x13, 0x15, 0x18, 0x1A, 0x1D, 0x1F, 0x22, 0x25, 0x28, 0x2B, 0x2E, 0x31, 0x34, 0x37, 0x3A, 0x3E, 0x41, 0x45, 0x48, 0x4C, 0x50, 0x53, 0x57, 0x5B,
        0x5F, 0x63, 0x67, 0x6B, 0x6F, 0x73, 0x77, 0x7B
};

const uint8_t SIN_TRANSFORM_1200_TO_2200[] PROGMEM = {
        0, 0, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 19, 19, 19, 20, 21, 21, 22, 22, 31, 23, 24, 30, 25, 26, 26, 25, 26, 26,
        26, 26, 30, 30, 31, 31, 31, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 39, 39, 40, 40, 41, 41, 42, 43, 43, 44, 44, 45, 45, 46, 46, 47, 47, 48, 48, 49, 50, 50, 51, 51, 52, 52, 53, 53, 54,
        54, 55, 56, 56, 57, 57, 58, 58, 59, 59, 60, 61, 61, 62, 62, 63, 63, 64, 64, 65, 65, 66, 66, 67, 67, 68, 69, 69, 70, 70, 71, 71, 72, 72, 73, 74, 74, 75, 75, 76, 76, 76, 77, 77, 78, 79, 79, 80,
        80, 80, 80, 80, 80, 83, 79, 84, 85, 77, 86, 87, 87, 88, 88, 89, 89, 90, 90, 91, 91, 92, 93, 93, 94, 94, 95, 95, 96, 97, 97, 98, 98, 99, 99, 100, 100, 101, 101, 102, 102, 103, 104, 104, 105,
        105, 106, 106, 107, 107, 108, 109
};

const uint8_t SIN_2200_HZ[] PROGMEM = {
        0x7F, 0x86, 0x8D, 0x94, 0x9C, 0xA3, 0xAA, 0xB0, 0xB7, 0xBD, 0xC4, 0xCA, 0xD0, 0xD5, 0xDA, 0xDF, 0xE4, 0xE8, 0xEC, 0xEF, 0xF3, 0xF5, 0xF8, 0xFA, 0xFB, 0xFC, 0xFD, 0xFD, 0xFD, 0xFD, 0xFC, 0xFB,
        0xF9, 0xF7, 0xF4, 0xF1, 0xEE, 0xEA, 0xE6, 0xE1, 0xDD, 0xD8, 0xD2, 0xCD, 0xC7, 0xC1, 0xBA, 0xB4, 0xAD, 0xA6, 0x9F, 0x98, 0x91, 0x89, 0x82, 0x7B, 0x74, 0x6C, 0x65, 0x5E, 0x57, 0x50, 0x49, 0x43,
        0x3C, 0x36, 0x30, 0x2B, 0x25, 0x20, 0x1C, 0x17, 0x13, 0x0F, 0x0C, 0x09, 0x06, 0x04, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x05, 0x08, 0x0A, 0x0E, 0x11, 0x15, 0x19, 0x1E, 0x23,
        0x28, 0x2D, 0x33, 0x39, 0x40, 0x46, 0x4D, 0x53, 0x5A, 0x61, 0x69, 0x70, 0x77, 0x7E, 0x86, 0x8D, 0x94, 0x9C, 0xA3, 0xAA, 0xB0, 0xB7, 0xBD, 0xC4, 0xCA, 0xD0, 0xD5, 0xDA, 0xDF, 0xE4, 0xE8, 0xEC,
        0xEF, 0xF3, 0xF5, 0xF8, 0xFA, 0xFB, 0xFC, 0xFD, 0xFD, 0xFD, 0xFD, 0xFC, 0xFB, 0xF9, 0xF7, 0xF4, 0xF1, 0xEE, 0xEA, 0xE6, 0xE1, 0xDD, 0xD8, 0xD2, 0xCD, 0xC7, 0xC1, 0xBA, 0xB4, 0xAD, 0xA6, 0x9F,
        0x98, 0x91, 0x89, 0x82, 0x7B, 0x74, 0x6C, 0x65, 0x5E, 0x57, 0x50, 0x49, 0x43, 0x3C, 0x36, 0x30, 0x2B, 0x25, 0x20, 0x1C, 0x17, 0x13, 0x0F, 0x0C, 0x09, 0x06, 0x04, 0x02, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x02, 0x03, 0x05, 0x08, 0x0A, 0x0E
};

const uint8_t SIN_TRANSFORM_2200_TO_1200[] PROGMEM = {
        0, 2, 4, 5, 7, 9, 11, 13, 15, 16, 18, 20, 22, 24, 26, 27, 29, 31, 33, 34, 37, 38, 41, 43, 44, 46, 47, 47, 47, 47, 45, 56, 42, 60, 62, 64, 66, 68, 70, 72, 73, 75, 77, 79, 81, 82, 84, 86, 88,
        90, 92, 93, 95, 97, 99, 101, 103, 105, 106, 108, 110, 112, 114, 115, 117, 119, 121, 123, 125, 127, 128, 130, 132, 134, 136, 137, 140, 141, 144, 145, 147, 147, 147, 153, 155, 156, 158, 159,
        162, 163, 165, 167, 169, 170, 172, 174, 176, 178, 180, 182, 184, 185, 187, 189, 191, 192, 194, 196, 198, 0, 2, 4, 5, 7, 9, 11, 13, 15, 16, 18, 20, 22, 24, 26, 27, 29, 31, 33, 34, 37, 38, 41,
        43, 44, 46, 47, 47, 47, 47, 45, 56, 42, 60, 62, 64, 66, 68, 70, 72, 73, 75, 77, 79, 81, 82, 84, 86, 88, 90, 92, 93, 95, 97, 99, 101, 103, 105, 106, 108, 110, 112, 114, 115, 117, 119, 121, 123,
        125, 127, 128, 130, 132, 134, 136, 137, 140, 141, 144, 145, 147, 147, 147, 153, 155, 156, 158, 159, 162, 163, 165,
};

static volatile bool m_transmitActive = false;                          ///< Tracks if the IRS is active
static uint8_t m_nextSinIndex = 0;
static uint8_t m_sinBitsLeft = 200;
static bool m_currently1200 = true;
static int32_t m_bitIndex = 0;
static int32_t m_totalBitNum = 0;
static const uint8_t* m_transmitData = nullptr;

// Based on: https://github.com/ocrdu/Arduino_SAMD21_Audio_Player/blob/master/src/AudioPlayer.cpp
// License: GNU General Public License v3.0 (same as project)

void AprsTransmitter::setup() {
    uint32_t top = 47972352 / SAMPLE_FREQUENCY;             // Calculate the TOP value
    REG_GCLK_GENDIV = GCLK_GENDIV_DIV(1) |                  // Divide the 48MHz clock source by 1 for 48MHz
                      GCLK_GENDIV_ID(3);                    // Select GCLK3
    while (GCLK->STATUS.bit.SYNCBUSY);                      // Wait for synchronization

    REG_GCLK_GENCTRL = GCLK_GENCTRL_IDC |                   // Set the duty cycle to 50/50
                       GCLK_GENCTRL_GENEN |                 // Enable GCLK3
                       GCLK_GENCTRL_SRC_DFLL48M |           // Set the 48MHz clock source
                       GCLK_GENCTRL_ID(3);                  // Select GCLK3
    while (GCLK->STATUS.bit.SYNCBUSY);                      // Wait for synchronization

    REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |                 // Enable clock
                       GCLK_CLKCTRL_GEN_GCLK3 |             // Select GCLK3
                       GCLK_CLKCTRL_ID_TC4_TC5;             // Feed the GCLK3 to TC4 and TC5
    while (GCLK->STATUS.bit.SYNCBUSY);                      // Wait for synchronization

    REG_TC4_COUNT16_CC0 = top;                              // Set the TC4 CC0 register to top
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);               // Wait for synchronization

    NVIC_SetPriority(TC4_IRQn, 0);                          // Set the interrupt priority for TC4 to 0 (highest)
    NVIC_EnableIRQ(TC4_IRQn);                               // Connect TC4 to Nested Vector Interrupt Controller

    REG_TC4_INTFLAG |= TC_INTFLAG_OVF;                      // Clear the interrupt flags
    REG_TC4_INTENSET = TC_INTENCLR_OVF;                     // Enable TC4 interrupts

    REG_TC4_CTRLA |= TC_CTRLA_PRESCALER_DIV1 |              // Set prescaler to 1 for 48MHz
                     TC_CTRLA_WAVEGEN_MFRQ;                 // Put the timer TC4 into Match Frequency Mode
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);               // Wait for synchronization
}

inline void DACWrite(uint16_t sample) {
    sample *= 257;
//    Serial.println(int(sample));
    DAC->DATA.reg = sample;                               // Shortened version
    while (ADC->STATUS.bit.SYNCBUSY);                     // of the code used
    DAC->CTRLA.bit.ENABLE = 0x01;                         // in analogWrite()
    while (ADC->STATUS.bit.SYNCBUSY);                     // in wiring_analog.c
}

void AprsTransmitter::endTransmission() {
    REG_TC4_CTRLA &= ~TC_CTRLA_ENABLE;                  // Disable timer TC4
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);           // Wait for synchronization
    REG_TC4_INTFLAG = TC_INTFLAG_OVF;
    DACWrite(0);
    m_transmitActive = false;
}

// Interrupt Service Routine for timer TC4
void TC4_Handler() {
    // Check if we have completed the sin wave for the current bit
    if (m_sinBitsLeft <= 0) {
        m_sinBitsLeft = 200;
        // Move to the next bit
        m_bitIndex++;
        // Check if we have finished the entire transmission
        if (m_bitIndex >= m_totalBitNum) {
            AprsTransmitter::endTransmission();
            return;
        }

        // Check if we need to switch frequency's/sin tables. Note that a 1 is 1200hz
        if (((m_transmitData[m_bitIndex / 8] >> (7 - (m_bitIndex % 8))) & 0x01) != m_currently1200) {
            // Get the index in the new tone's sin table
            if (m_currently1200) {
                m_nextSinIndex = pgm_read_byte(&SIN_TRANSFORM_1200_TO_2200[m_nextSinIndex]);
            } else {
                m_nextSinIndex = pgm_read_byte(&SIN_TRANSFORM_2200_TO_1200[m_nextSinIndex]);
            }
            // Switch the current frequency/sin table
            m_currently1200 = !m_currently1200;
        }
    }

    // Write the current DAC value
    if (m_currently1200) {
        DACWrite(pgm_read_byte(&SIN_1200_HZ[m_nextSinIndex]));
    } else {
        DACWrite(pgm_read_byte(&SIN_2200_HZ[m_nextSinIndex]));
    }

    // Increment the index in the sin table
    m_nextSinIndex++;
    m_sinBitsLeft--;
    if (m_nextSinIndex >= sizeof(SIN_1200_HZ)) {
        if(m_currently1200) {
            m_nextSinIndex = 0;
        } else {
            m_nextSinIndex = 91;
        }

    }
    // Clear the OVF interrupt flag
    REG_TC4_INTFLAG = TC_INTFLAG_OVF;
}

void AprsTransmitter::send(const uint8_t* data, int32_t bitNum) {
//    m_tr2ansmitData;
    // Prevent transmissions when one is already active
    if (m_transmitActive) return;
    m_transmitActive = true;

    DACWrite(uint16_t(-1));
    delay(10);
    DACWrite(uint16_t(0));
    delay(10);
    DACWrite(uint16_t(-1));
    delay(10);
    DACWrite(uint16_t(0));
    delay(10);
    DACWrite(uint16_t(-1));
    delay(10);
    DACWrite(uint16_t(0));
    delay(10);

    // Save the data for sending
    m_transmitData = data;
    m_bitIndex = -1;
    m_totalBitNum = bitNum;
    // Reset indices
    m_nextSinIndex = 0;
    m_sinBitsLeft = 0;

    // Enable to interrupt to allow for modulation
    REG_TC4_CTRLA |= TC_CTRLA_ENABLE;                       // Enable timer TC4
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);               // Wait for synchronization

    // Wait for the transmission to complete (can eventually remove) @TODO remove
    while (isCurrentlyTransmitting());
}

bool AprsTransmitter::isCurrentlyTransmitting() {
    return m_transmitActive;
}


