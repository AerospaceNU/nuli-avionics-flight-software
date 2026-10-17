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

    // Continuous high-rate injection, so high-bandwidth only (like --streamLog).
    SimulationParser() : m_simFlag("--sim", "Injects sim sensor values", true, [this](DebugStream*) { this->simCallback(); }, true) {}

    void setup(IntegratedParser* parser, DebugStream* debug, HardwareAbstraction* hardware) {
        m_parser = parser;
        m_debug = debug;
        m_watchdog = &hardware->getWatchdogTimer();
        m_parser->addFlagGroup(m_simGroup);
    }

    void waitForEntry() const {
        // *Sim envs run on real hardware, so an un-pet wait here would reset-loop the board.
        while (m_simDataBuffer.empty()) {
            m_watchdog->petInLoop();
            m_parser->runCli();
        }
        // Iteration count must be >= python's BURST to absorb its per-report burst.
        for (int i = 0; i < 2; i++) {
            m_parser->runCli();
        }
        // Reported after draining so python's flow control sees the true queue level.
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
