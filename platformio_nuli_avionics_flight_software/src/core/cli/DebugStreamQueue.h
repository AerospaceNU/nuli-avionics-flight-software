#ifndef DEBUGSTREAMQUEUE_H
#define DEBUGSTREAMQUEUE_H

#include "core/generic_hardware/DebugStream.h"
#include "etl/queue.h"

// A DebugStream backed by byte queues instead of a real transport - lets a caller feed it
// received bytes (pushIncoming) and drain bytes it has written (popOutgoing) on its own
// schedule, e.g. a radio link that only gets to send/receive on a periodic packet cadence
// instead of a continuous serial stream. Handing this to Parser::addStream() is what makes
// CLI commands/responses flow over that transport with no Parser/IntegratedParser changes.
// Leaves m_isHighBandwidth at DebugStream's default (false) - this is exactly the kind of slow,
// relayed link highBandwidthOnly-flagged commands (--offload, --streamLog, ...) need to skip.
template<size_t InputSize, size_t OutputSize>
class DebugStreamQueue : public DebugStream {
public:
    // Called by the owner when a received message carrying a CLI command comes in. Drops
    // bytes past capacity rather than blocking/asserting - a too-long command just fails to
    // parse, same as it would if truncated over a flaky serial link.
    void pushIncoming(const uint8_t* data, uint8_t length) {
        for (uint8_t i = 0; i < length && m_input.size() + 1 < InputSize; i++) m_input.push(data[i]);
        if (!m_input.full()) m_input.push('\0');
    }

    // Called by the owner right before it's about to send, to claim up to maxLength bytes
    // for this cycle's packet. Returns the number of bytes actually written to outBuffer.
    uint8_t popOutgoing(uint8_t* outBuffer, uint8_t maxLength) {
        uint8_t n = 0;
        while (n < maxLength && !m_output.empty()) {
            outBuffer[n++] = m_output.front();
            m_output.pop();
        }
        return n;
    }

    bool hasOutgoing() const { return !m_output.empty(); }

    bool readLine() override {
        if (m_lineReady) return true;
        while (!m_input.empty()) {
            char c = (char)m_input.front();
            m_input.pop();
            if (c == '\0') {
                m_lineBuffer[m_lineLen] = '\0';
                m_lineLen = 0;
                m_lineReady = true;
                return true;
            }
            if (m_lineLen < sizeof(m_lineBuffer) - 1) m_lineBuffer[m_lineLen++] = c;
        }
        return false;
    }

    char* getLine() override {
        m_lineReady = false;
        return m_lineBuffer;
    }

protected:
    // Overrides DebugStream::write() - every message()/warn()/error()/data() call ends up
    // here, so this is the sole place bytes enter the output queue.
    size_t write(const void* buffer, size_t size) override {
        const uint8_t* p = (const uint8_t*)buffer;
        size_t n = 0;
        for (; n < size && !m_output.full(); ++n) m_output.push(p[n]);
        return n; // silently drops the tail once full, same backpressure semantics as a USB write()
    }

private:
    etl::queue<uint8_t, InputSize> m_input;
    etl::queue<uint8_t, OutputSize> m_output;
    char m_lineBuffer[128] = {};
    uint8_t m_lineLen = 0;
    bool m_lineReady = false;
};

#endif //DEBUGSTREAMQUEUE_H
