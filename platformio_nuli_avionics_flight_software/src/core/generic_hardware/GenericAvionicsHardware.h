#ifndef AVIONICSHARDWARE_H
#define AVIONICSHARDWARE_H

#include "DebugStream.h"

class WatchdogTimer;

class GenericAvionicsHardware {
public:
    virtual ~GenericAvionicsHardware() = default;
    virtual void setup(DebugStream* debugStream, WatchdogTimer* watchdog) {}
    virtual void run() {}
    virtual void read() {}
};


#endif //AVIONICSHARDWARE_H
