#include "HardwareAbstraction.h"
#include <cmath>

#include "drivers/arduino/ArduinoSerialReader.h"

HardwareAbstraction::HardwareAbstraction(DebugStream& debugStream, SystemClock& systemClock, uint32_t loopRateHz) {
    m_debug = &debugStream;
    m_systemClock = &systemClock;
    setLoopRateHz(loopRateHz);
}

void HardwareAbstraction::setup() {
    m_debug->setup();
    m_debug->message("SETTING UP HARDWARE");
    m_systemClock->setup(m_debug);
    for (GenericAvionicsHardware* hardwareDevice : m_allHardware) hardwareDevice->setup(m_debug);
    m_debug->message("HARDWARE SET UP COMPLETE\r\n");
}

void HardwareAbstraction::runAndReadAllHardware() const {
    for (GenericAvionicsHardware* hardwareDevice : m_allHardware) {
        hardwareDevice->run();
        hardwareDevice->read();
    }
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
            while ((int32_t)(m_intendedTickEndtimeUs - actualLoopEndtimeUs) > 0 && int32_t(m_intendedTickEndtimeUs - actualLoopEndtimeUs) < int32_t(m_loopDtMs * Units::MS_TO_US)) {
                actualLoopEndtimeUs = m_systemClock->currentRuntimeUs();
            }
            m_intendedTickEndtimeUs += (m_loopTime * Units::MS_TO_US);
            m_loopDtMs = m_loopTime;
        }
    }

    m_currentLoopTimestampMs = m_systemClock->currentRuntimeMs();
    m_tickCount++;
    // Return a new timestamp
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

void HardwareAbstraction::avionicsSystemError(const char* message) const {
    m_debug->setup();
    m_debug->error("Critical Avionics Issue: %s", message);
    while (AVIONICS_ARGUMENT_isDev) {}
}
