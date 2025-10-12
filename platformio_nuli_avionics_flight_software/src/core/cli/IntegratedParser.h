#ifndef INTEGRATEDPARSER_H
#define INTEGRATEDPARSER_H

#include  "Parser.h"
#include "core/generic_hardware/LineReader.h"
#include "core/generic_hardware/DebugStream.h"

class IntegratedParser : public Parser {
public:
    void setup(LineReader* lineReader, DebugStream* debugStream) {
        m_lineReader = lineReader;
        m_debugStream = debugStream;
    };

    void runCli() {
        if (m_lineReader->readLine()) {
            const int errorCode = parse(m_lineReader->getLine());
            if (errorCode == 0) {
                runFlags();
                resetFlags();
            } else {
                m_debugStream->error("Invalid message: %d, %s", errorCode, m_lineReader->getLine());
            }
        }
    }

private:
    LineReader* m_lineReader = nullptr;
    DebugStream* m_debugStream = nullptr;
};

#endif //INTEGRATEDPARSER_H
