#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSTRANSMITTER_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSTRANSMITTER_H

// Based on: https://github.com/ocrdu/Arduino_SAMD21_Audio_Player/blob/master/src/AudioPlayer.cpp
// License: GNU General Public License v3.0 (same as project)

#include <cstdint>

/**
 * @brief AFSK-modulates bits onto the SAMD21 DAC from 1200/2200 Hz sine tables
 * @details A tone switch starts the new table at the same value and direction, keeping the phase continuous.
 */
class AprsTransmitter {
public:
    /** @brief Sets up the TC4 sample timer; call once, first */
    static void setup();

    /** @param bitNum Number of bits (not bytes) to send */
    static void send(const uint8_t* data, int32_t bitNum);


    static inline void endTransmission();

    static bool isCurrentlyTransmitting();

    static constexpr uint32_t BOD_RATE = 1200;                      ///< bits/s
    static constexpr uint32_t SAMPLE_FREQUENCY = 240000;            ///< samples/s
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_APRSTRANSMITTER_H
