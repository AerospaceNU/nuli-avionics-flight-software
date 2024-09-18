#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSTRANSMITTER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSTRANSMITTER_H

// Based on: https://github.com/ocrdu/Arduino_SAMD21_Audio_Player/blob/master/src/AudioPlayer.cpp
// License: GNU General Public License v3.0 (same as project)

#include <cstdint>

/**
 * @struct BitSinTable_s
 * @brief Holds a reference to the segment of a sin waveform representing a single bit of data
 */
struct BitSinTable_s {
    const uint8_t* start;           ///< Pointer to somewhere in one of the pre-generated sin tables
    uint32_t length;                ///< How long of a segment to use from the table. Should always be SAMPLE_FREQUENCY / BOD_RATE
};

/**
 * @class AprsTransmitter
 * @brief Turns binary data into sin waves, output-ed on the DAC
 * @details To modulate the signal, precomputed 1200 hz and 2200 hz sin tables were computed. 2200 represents a one and 1200 represents a zero.
 * To provide a smooth transition between the two frequency's, you must start the next frequency at the same value and direction as the current one.
 * This class handles computing where to start in each sin table for each bit, and then uses timers/interrupts to accurately modulate the DAC/
 */
class AprsTransmitter {
public:
    /**
     * @brief Initializes the hardware timers needed for modulating the signal accurately. Must be called only once, before any other methods are called.
     */
    static void configure();

    /**
     * @brief Transmits binary data through the DAC
     * @param data Binary data to transmit
     * @param length How many bytes to transmit
     */
    static void send(const uint8_t* data, int32_t length);

    /**
     * @brief Begins a transition of the last data sent
     */
    static void beginTransmission();

    /**
     * Returns if the signal modulation is active
     * @return if the signal modulation is active
     */
    static bool isCurrentlyTransmitting();

    // Member variables are public because the IRS needs to access them
    static constexpr uint32_t BOD_RATE = 1200;                      ///< APRS bod rate in bits/second
    static constexpr uint32_t SAMPLE_FREQUENCY = 240000;            ///< Sample frequency of the sin tables in samples/second
    static constexpr uint32_t MAXIMUM_BITS = 500;                   ///< Maximum message length in bits

    static volatile bool m_transmitActive;                          ///< Tracks if the IRS is active
    // The current bit's waveform
    static const uint8_t* m_currentSample;                          ///< Pointer to the currently active sin table (might not point at the start of the table)
    static uint32_t m_currentSampleSize;                            ///< Length of the current sample (should always be SAMPLE_FREQUENCY / BOD_RATE)
    static uint16_t currentSampleValue;                             ///< @todo is this needed to be a member? Can probably be local or deleted
    static uint32_t sampleIndex;                                    ///< Index within the sin table
    // Array of all bit's waveforms
    static uint32_t bitIndex;                                       ///< Which bit we are currently sending
    static uint32_t m_transmissionBitWaveformsNum;                  ///< How many bits we have to send
    static BitSinTable_s m_transmissionBitWaveforms[MAXIMUM_BITS];  ///< Datastructure containing the sin table windows for every bit we will transmit
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSTRANSMITTER_H
