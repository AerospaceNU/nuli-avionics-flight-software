#ifndef SIMULATIONPARSER_H
#define SIMULATIONPARSER_H

#include <cstdlib>
#include "IntegratedParser.h"
#include "ArgumentFlag.h"
#include "etl/circular_buffer.h"

template <unsigned N>
class SimulationParser {
public:
    static constexpr unsigned BUFFER_CAPACITY = 100;

    SimulationParser() : m_simFlag("--sim", "Inject comma/space-separated float values into the sim buffer", true, 255, [this]() { this->simCallback(); }) {}

    void setup(IntegratedParser* parser, DebugStream* debug) {
        m_parser = parser;
        m_debug = debug;
        m_parser->addFlagGroup(m_simGroup);
    }

    void waitForEntry() const {
        // Drain at least one line (block if buffer is empty)
        while (m_simDataBuffer.empty()) {
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
};

#endif //SIMULATIONPARSER_H
