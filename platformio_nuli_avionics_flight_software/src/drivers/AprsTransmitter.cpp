#include "AprsTransmitter.h"
#include <Arduino.h>
#include <SinTables.h>

// Based on: https://github.com/ocrdu/Arduino_SAMD21_Audio_Player/blob/master/src/AudioPlayer.cpp
// License: GNU General Public License v3.0 (same as project)

volatile bool AprsTransmitter::m_transmitActive = false;
// The current bit's waveform
 const uint8_t* AprsTransmitter::m_currentSample = nullptr;
 uint32_t AprsTransmitter::m_currentSampleSize = 0;
 uint16_t AprsTransmitter::currentSampleValue = 0;
 uint32_t AprsTransmitter::sampleIndex = 0;
// Array of all bit's waveforms
 uint32_t AprsTransmitter::bitIndex = 0;
 uint32_t AprsTransmitter::m_transmissionBitWaveformsNum = 0;
 BitSinTable_s AprsTransmitter::m_transmissionBitWaveforms[MAXIMUM_BITS] = {};

void AprsTransmitter::configure() {
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

void AprsTransmitter::beginTransmission() {
    // Prevent invalid transitions
    if (m_transmitActive || m_transmissionBitWaveformsNum <= 0) return;
    m_transmitActive = true;
    // Reset all tracker variables and preload the first bit's waveform
    bitIndex = 0;
    sampleIndex = 0;
    m_currentSample = m_transmissionBitWaveforms[bitIndex].start;
    m_currentSampleSize = m_transmissionBitWaveforms[bitIndex].length;

    REG_TC4_CTRLA |= TC_CTRLA_ENABLE;                       // Enable timer TC4
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);               // Wait for synchronization
}

inline void DACWrite(uint16_t sample) {
    DAC->DATA.reg = sample;                               // Shortened version
    while (ADC->STATUS.bit.SYNCBUSY);                     // of the code used
    DAC->CTRLA.bit.ENABLE = 0x01;                         // in analogWrite()
    while (ADC->STATUS.bit.SYNCBUSY);                     // in wiring_analog.c
}

void TC4_Handler() {                                                                // Interrupt Service Routine for timer TC4
    AprsTransmitter::currentSampleValue = AprsTransmitter::m_currentSample[AprsTransmitter::sampleIndex];       // Get the current sample value
    AprsTransmitter::currentSampleValue <<= 2;                                                  // Go to 10 bits for calculations and for DAC

    DACWrite(AprsTransmitter::currentSampleValue);                                              // Send the current sample to the DAC
    AprsTransmitter::sampleIndex++;                                                       // Go to the next sample
    // Check if we have finished the bit
    if (AprsTransmitter::sampleIndex >= AprsTransmitter::m_currentSampleSize) {                         // At the end of the samples array:
        // Increment the bit
        AprsTransmitter::bitIndex++;
        // Check if we have finished the transmission
        if (AprsTransmitter::bitIndex >= AprsTransmitter::m_transmissionBitWaveformsNum) {
            REG_TC4_CTRLA &= ~TC_CTRLA_ENABLE;                  // Disable timer TC4
            while (TC4->COUNT16.STATUS.bit.SYNCBUSY);           // Wait for synchronization
            DACWrite(0);
            AprsTransmitter::m_transmitActive = false;
        } else {
            // Set up the next bit
            AprsTransmitter::sampleIndex = 0;
            AprsTransmitter::m_currentSample = AprsTransmitter::m_transmissionBitWaveforms[AprsTransmitter::bitIndex].start;
            AprsTransmitter::m_currentSampleSize = AprsTransmitter::m_transmissionBitWaveforms[AprsTransmitter::bitIndex].length;
        }
    }

    REG_TC4_INTFLAG = TC_INTFLAG_OVF;                       // Clear the OVF interrupt flag
}

const uint8_t* determineStart(const uint8_t* sinTable, uint32_t searchLength, uint8_t previousEndpointValue, int8_t previousEndpointSlopeSign) {
    const uint8_t* bestStartPosition = sinTable;
    for (uint32_t i = 0; i < searchLength - 1; i++) {
        // Figure out if the slope between this datapoint and the next datapoint is positive
        int8_t slopeSign = (sinTable[i] < sinTable[i + 1]) ? 1 : -1;
        // Check for the case where the sine wave have the same value. We want the closest value that is not the same
        if (slopeSign == previousEndpointSlopeSign && sinTable[i] == previousEndpointValue) {
            return &sinTable[i];
        }
        // Figure out if this datapoint is closer than the best one found so far
        bool closerThanBest = abs(sinTable[i] - previousEndpointValue) < abs(bestStartPosition[0] - previousEndpointValue);
        // If the datapoint is the closest so far, and the next datapoint is in the correct direction
        if (slopeSign == previousEndpointSlopeSign && closerThanBest) {
            bestStartPosition = &sinTable[i];
        }
    }
    return bestStartPosition;
}

void AprsTransmitter::send(const uint8_t* data, int32_t length) {
    ////// Check for max length
    const uint32_t samplesPerBit = SAMPLE_FREQUENCY / BOD_RATE;     // This ideally comes out to an even number. 240000/1200 works nicely
    // Track the sin wave at the end of each bit, so that the next bit can smoothly transition
    uint8_t previousEndpointValue = 0;
    int8_t previousSlopeSign = 1;

    // Iterate over each byte to send
    for (int32_t i = 0; i < length; i++) {
        uint8_t currentByte = data[i];
        // Iterate over each bit within the byte
        for (int bit = 7; bit >= 0; --bit) {
            bool bitValue = (currentByte >> bit) & 1;

            // Determine where to start in the sin wave to ensure a smooth transition
            const uint8_t* startPoint;
            if (bitValue) {
                startPoint = determineStart(Sine2P2kHz, samplesPerBit + 1, previousEndpointValue, previousSlopeSign);
            } else {
                startPoint = determineStart(Sine1P2kHz, samplesPerBit + 1, previousEndpointValue, previousSlopeSign);
            }

            // Record the startpoint to an array
            m_transmissionBitWaveforms[m_transmissionBitWaveformsNum] = {startPoint, samplesPerBit};
            m_transmissionBitWaveformsNum++;

            // Store information about the end of this bit's waveform to best select the start of the next bit
            previousEndpointValue = startPoint[samplesPerBit - 1];
            previousSlopeSign = (startPoint[samplesPerBit - 2] < startPoint[samplesPerBit - 1]) ? 1 : -1;
        }
    }

    beginTransmission();
}

bool AprsTransmitter::isCurrentlyTransmitting() {
    return m_transmitActive;
}
