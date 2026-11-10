#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPPYRO_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPPYRO_H

#include "Avionics.h"
#include "core/generic_hardware/Pyro.h"
#include "core/generic_hardware/SystemClock.h"
#include "util/Timer.h"

class DesktopPyro final : public Pyro {
public:
    explicit DesktopPyro(SystemClock& clock) : m_clock(clock) {}

    void run() override {
        if (m_timedFireAlarm.isInitialized() && m_timedFireAlarm.isAlarmFinished(m_clock.currentRuntimeMs())) {
            disable();
            m_timedFireAlarm.reset();
        }
    }

    bool isFired() const override { return m_isFired; }

    void disable() override { m_isFired = false; }

protected:
    void turnOnOutput() override {
        m_isFired = true;
        m_timedFireAlarm.reset();
    }

    void fireOutputFor(const uint32_t timeMs) override {
        turnOnOutput();
        m_timedFireAlarm.startAlarm(m_clock.currentRuntimeMs(), timeMs);
    }

private:
    SystemClock& m_clock;
    Alarm m_timedFireAlarm;
    bool m_isFired = false;
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_DESKTOPPYRO_H
