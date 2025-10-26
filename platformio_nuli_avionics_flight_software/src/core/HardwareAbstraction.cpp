#include "HardwareAbstraction.h"
#include <cmath>

void HardwareAbstraction::setup(DebugStream* debugStream, SystemClock* systemClock, const uint32_t loopRateHz) {
    m_debug = debugStream;
    m_systemClock = systemClock;
    setLoopRateHz(loopRateHz);
    // Setup core
    if (m_debug == nullptr || m_systemClock == nullptr) {
        if (m_debug != nullptr) {
            m_debug->setup();
            m_debug->error("Debug stream, clock, configuration, and configuration memory required");
        }
        while (true);
    }
    m_debug->setup();
    m_debug->message("SETTING UP HARDWARE");
    m_systemClock->setup(m_debug);

    for (int i = 0; i < m_numPyros; i++) m_pyroArray[i]->setup(m_debug);
    for (int i = 0; i < m_numVoltageSensors; i++) m_voltageSensorArray[i]->setup(m_debug);
    for (int i = 0; i < m_numBarometers; i++) m_barometerArray[i]->setup(m_debug);
    for (int i = 0; i < m_numAccelerometers; i++) m_accelerometerArray[i]->setup(m_debug);
    for (int i = 0; i < m_numMagnetometers; i++) m_magnetometerArray[i]->setup(m_debug);
    for (int i = 0; i < m_numGyroscopes; i++) m_gyroscopeArray[i]->setup(m_debug);
    for (int i = 0; i < m_numFlashMemory; i++) m_flashMemoryArray[i]->setup(m_debug);
    for (int i = 0; i < m_numFramMemory; i++) m_framMemoryArray[i]->setup(m_debug);
    for (int i = 0; i < m_numRadioLinks; i++) m_radioLinkArray[i]->setup(m_debug);
    for (int i = 0; i < m_numIndicators; i++) m_indicatorArray[i]->setup(m_debug);
    for (int i = 0; i < m_numGenericSensors; i++) m_genericSensorArray[i]->setup(m_debug);

    m_debug->message("HARDWARE SET UP COMPLETE\r\n");
}

void HardwareAbstraction::readSensors() const {
    for (int i = 0; i < m_numPyros; i++) m_pyroArray[i]->read();
    for (int i = 0; i < m_numVoltageSensors; i++) m_voltageSensorArray[i]->read();
    for (int i = 0; i < m_numBarometers; i++) m_barometerArray[i]->read();
    for (int i = 0; i < m_numAccelerometers; i++) m_accelerometerArray[i]->read();
    for (int i = 0; i < m_numMagnetometers; i++) m_magnetometerArray[i]->read();
    for (int i = 0; i < m_numGyroscopes; i++) m_gyroscopeArray[i]->read();
    for (int i = 0; i < m_numGenericSensors; i++) m_genericSensorArray[i]->read();
}

void HardwareAbstraction::setLoopRateHz(const uint32_t loopRate) {
    const double delay = 1000.0 / loopRate;
    m_loopTime = std::lround(delay);
}

Timestamp_s HardwareAbstraction::enforceLoopTime() {
    uint32_t actualLoopEndtimeUs = m_systemClock->currentRuntimeUs();

    if (m_tickCount == 0) {
        m_loopDtMs = m_loopTime;
        m_debug->message("First loop start time: %d", m_systemClock->currentRuntimeMs());
        m_intendedTickEndtimeUs = actualLoopEndtimeUs + (m_loopTime * Units::MS_TO_US);
    } else {
        const uint32_t diff = actualLoopEndtimeUs - m_intendedTickEndtimeUs;

        if ((int32_t)diff > 0) { // overrun
            m_loopDtMs = (diff + (m_loopTime * Units::MS_TO_US)) / Units::MS_TO_US;
            m_debug->warn("Loop overrun, lasting %d ms", m_loopDtMs);
            m_intendedTickEndtimeUs = actualLoopEndtimeUs + (m_loopTime * Units::MS_TO_US);
        } else {
            while ((int32_t)(m_intendedTickEndtimeUs - actualLoopEndtimeUs) > 0) {
                actualLoopEndtimeUs = m_systemClock->currentRuntimeUs();
            }
            m_intendedTickEndtimeUs += (m_loopTime * Units::MS_TO_US);
            m_loopDtMs = m_loopTime;
        }
    }

    m_currentLoopTimestampMs = m_systemClock->currentRuntimeMs();
    m_tickCount++;
    return getTimestamp();
}

Timestamp_s HardwareAbstraction::getTimestamp() const {
    Timestamp_s timestamp{};
    timestamp.runtime_ms = m_currentLoopTimestampMs;
    timestamp.dt_ms = m_loopDtMs;
    timestamp.tick = m_tickCount;
    return timestamp;
}

uint32_t HardwareAbstraction::getTargetLoopTimeMs() const {
    return m_loopTime;
}

DebugStream* HardwareAbstraction::getDebugStream() const { return m_debug; }


// // @todo ensure more reliable tick time using microseconds()
// // Track the end of the tick
// const uint32_t actualLoopEnd = m_systemClock->currentRuntimeMs();
// const uint32_t desiredLoopEnd = m_currentLoopTimestampMs + m_loopTime;
// // Enforce loop time, detect overruns
// if (actualLoopEnd > desiredLoopEnd) {
//     if (m_tickCount == 0) {
//         m_debug->message("First loop start time: %d", actualLoopEnd);
//     } else {
//         m_debug->warn("Loop overrun by %d ms", (actualLoopEnd - desiredLoopEnd));
//     }
// } else {
//     while (m_systemClock->currentRuntimeMs() < desiredLoopEnd) {};
// }
// // Update timers
// const uint32_t lastTime = m_currentLoopTimestampMs;
// m_currentLoopTimestampMs = m_systemClock->currentRuntimeMs();
// m_loopDtMs = m_currentLoopTimestampMs - lastTime;
// m_tickCount++;
// return getTimestamp();
