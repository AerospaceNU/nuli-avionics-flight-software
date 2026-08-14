#ifndef INTEGRATEDPARSER_H
#define INTEGRATEDPARSER_H

#include  "Parser.h"
#include "core/generic_hardware/GenericHardware.h"
#include "etl/vector.h"

class IntegratedParser : public Parser {
public:
    void addStream(DebugStream* debugStream) {
        m_debugStreams.push_back(debugStream);
    };

    void runCli() {
        for (auto stream: m_debugStreams) {
            if (stream->readLine()) {
                const int errorCode = parse(stream->getLine());
                if (errorCode == 0) {
                    runFlags(stream);
                    resetFlags();
                } else {
                    stream->error("Invalid message: %d, %s", errorCode, stream->getLine());
                }
            }
        }
    }

private:
    etl::vector<DebugStream*, 5> m_debugStreams;
};

#endif //INTEGRATEDPARSER_H
