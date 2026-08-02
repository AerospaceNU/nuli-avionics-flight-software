#ifndef SIMULATIONPARSER_H
#define SIMULATIONPARSER_H

#include <cstdlib>
#include "IntegratedParser.h"
#include "ArgumentFlag.h"
#include "core/generic_hardware/WatchdogTimer.h"
#include "core/HardwareAbstraction.h"
#include "etl/circular_buffer.h"

template <unsigned N>
class SimulationParser {
public:
    static constexpr unsigned BUFFER_CAPACITY = 100;

    // Continuous, high-rate sensor injection (fed from a host script) - same "no streaming"
    // category as BasicLogger's --streamLog, so restricted to high-bandwidth streams too.
    SimulationParser() : m_simFlag("--sim", "Injects sim sensor values", true, [this](DebugStream*) { this->simCallback(); }, true) {}

    void setup(IntegratedParser* parser, DebugStream* debug, HardwareAbstraction* hardware) {
        m_parser = parser;
        m_debug = debug;
        // Single instance, owned by HardwareAbstraction - not injected separately.
        m_watchdog = &hardware->getWatchdogTimer();
        m_parser->addFlagGroup(m_simGroup);
    }

    void waitForEntry() const {
        // Waits as long as the host harness takes, so pets every spin - *Sim envs extend the real
        // board envs (run on real hardware), so an un-pet wait here would reset-loop the board.
        while (m_simDataBuffer.empty()) {
            m_watchdog->petInLoop();
            m_parser->runCli();
        }
        // Absorb python's per-report burst. Should be >= python's BURST so the
        // post-drain report reflects every line python sent since the last one.
        for (int i = 0; i < 2; i++) {
            m_parser->runCli();
        }
        // Report buffer depth AFTER draining so python's flow control sees the
        // authoritative queue level — no in-flight accounting needed on its side.
        m_debug->data("--simSize %u", static_cast<unsigned>(m_simDataBuffer.size()));
    }

    float getValue(uint32_t index) {
        if (index >= N || m_simDataBuffer.empty()) return 0;
        return m_simDataBuffer.front().data[index];
    }

    void releaseEntry() {
        m_simDataBuffer.pop();
    }

    void simCallback() {
        SimDataEntry entry{};
        const char* p = m_simFlag.getValueDerived();
        for (uint32_t i = 0; i < N && p && *p != '\0'; i++) {
            char* end = nullptr;
            entry.data[i] = strtof(p, &end);
            if (end == p) break;
            p = end;
            while (*p == ',' || *p == ' ' || *p == '\t') p++;
        }
        m_simDataBuffer.push(entry);
    }

private:
    struct SimDataEntry {
        float data[N];
    };

    ArgumentFlag<const char*> m_simFlag;
    BaseFlag* m_simGroup[1] = {&m_simFlag};

    etl::circular_buffer<SimDataEntry, BUFFER_CAPACITY> m_simDataBuffer;
    IntegratedParser* m_parser = nullptr;
    DebugStream* m_debug = nullptr;
    WatchdogTimer* m_watchdog = nullptr;
};

#endif //SIMULATIONPARSER_H
