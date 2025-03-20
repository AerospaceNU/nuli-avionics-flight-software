#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSTRANSMITTER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSTRANSMITTER_H

// Based on: https://github.com/ocrdu/Arduino_SAMD21_Audio_Player/blob/master/src/AudioPlayer.cpp
// License: GNU General Public License v3.0 (same as project)

#include <cstdint>

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
    static void setup();

    /**
     * @brief Transmits binary data through the DAC
     * @param data Binary data to transmit
     * @param bitNum How many bytes to transmit
     */
    static void send(const uint8_t* data, int32_t bitNum);


    /**
     * @brief Begins a transition of the last data sent
     */
    static inline void endTransmission();

    /**
     * Returns if the signal modulation is active
     * @return if the signal modulation is active
     */
    static bool isCurrentlyTransmitting();

    // Member variables are public because the IRS needs to access them
    static constexpr uint32_t BOD_RATE = 1200;                      ///< APRS bod rate in bits/second
    static constexpr uint32_t SAMPLE_FREQUENCY = 240000;            ///< Sample frequency of the sin tables in samples/second
//    static constexpr uint32_t SAMPLE_FREQUENCY = 24000;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSTRANSMITTER_H
