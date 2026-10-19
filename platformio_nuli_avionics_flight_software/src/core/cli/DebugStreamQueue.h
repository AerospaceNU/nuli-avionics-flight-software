#ifndef DEBUGSTREAMQUEUE_H
#define DEBUGSTREAMQUEUE_H

#include "core/generic_hardware/DebugStream.h"
#include "etl/queue.h"

// DebugStream over byte queues for packet-cadence transports (e.g. radio); stays low-bandwidth.
template<size_t InputSize, size_t OutputSize>
class DebugStreamQueue : public DebugStream {
public:
    // Called by the owner when a received message carrying a CLI command comes in. Drops the whole
    // command if it doesn't fit.
    void pushIncoming(const uint8_t* data, uint8_t length) {
        if (m_input.size() + length + 1 > InputSize) return;
        for (uint8_t i = 0; i < length; i++) m_input.push(data[i]);
        m_input.push('\0');
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

    /** @brief Drops every received command that hasn't run yet. */
    void clearIncoming() {
        m_input.clear();
        m_lineLen = 0;
        m_lineReady = false;
        m_lineTooLong = false;
    }

    bool readLine() override {
        if (m_lineReady) return true;
        while (!m_input.empty()) {
            char c = (char)m_input.front();
            m_input.pop();
            if (c == '\0') {
                m_lineBuffer[m_lineLen] = '\0';
                m_lineLen = 0;
                if (m_lineTooLong) {
                    m_lineTooLong = false;
                    continue;
                }
                m_lineReady = true;
                return true;
            }
            if (m_lineLen < sizeof(m_lineBuffer) - 1) {
                m_lineBuffer[m_lineLen++] = c;
            } else {
                m_lineTooLong = true;
            }
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
    bool m_lineTooLong = false;
    uint8_t m_lineLen = 0;
    bool m_lineReady = false;
};

#endif //DEBUGSTREAMQUEUE_H
