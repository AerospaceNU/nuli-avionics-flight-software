#ifndef DUALDEPLOY_H
#define DUALDEPLOY_H

#include "Avionics.h"
#include "core/configuration/Configuration.h"
#include "core/generic_hardware/Pyro.h"
#include "core/state_estimation/FlightStateDeterminer.h"
#include "util/Timer.h"

/**
 * @class DualDeploy
 * @brief Fires the drogue DROGUE_DELAY after apogee, then the main once below MAIN_ELEVATION
 */
class DualDeploy {
public:
    constexpr static ConfigurationID_t REQUIRED_CONFIGS[] = {DROGUE_DELAY_c, MAIN_ELEVATION_c, PYRO_FIRE_DURATION_c};

    void setup(Configuration* configuration, Pyro* droguePyro, Pyro* mainPyro) {
        m_droguePyro = droguePyro;
        m_mainPyro = mainPyro;
        m_drogueDelay = configuration->getConfigurable<DROGUE_DELAY_c>();
        m_mainElevation = configuration->getConfigurable<MAIN_ELEVATION_c>();
        m_pyroFireDuration = configuration->getConfigurable<PYRO_FIRE_DURATION_c>();
    }

    void update(const RocketState_s& state, const FlightStateDeterminer& flightStateDeterminer) {
        if (state.flightState != DESCENT) return;
        if (flightStateDeterminer.getStateTimer()->getTimeElapsed(state.timestamp.runtime_ms) > m_drogueDelay.get() && !m_drogueFired) {
            m_droguePyro->fireFor(m_pyroFireDuration.get());
            m_drogueFired = true;
        }
        if (m_mainDeployDebounce.check(state.state1D.altitudeM <= m_mainElevation.get(), state.timestamp.runtime_ms) && m_drogueFired && !m_mainFired) {
            m_mainPyro->fireFor(m_pyroFireDuration.get());
            m_mainFired = true;
        }
    }

private:
    Pyro* m_droguePyro = nullptr;
    Pyro* m_mainPyro = nullptr;
    ConfigurationData<uint32_t> m_drogueDelay;
    ConfigurationData<float> m_mainElevation;
    ConfigurationData<uint32_t> m_pyroFireDuration;
    Debounce m_mainDeployDebounce = Debounce(MAIN_DEPLOY_DEBOUNCE_TIMER_MS);
    bool m_drogueFired = false;
    bool m_mainFired = false;
};

#endif //DUALDEPLOY_H
