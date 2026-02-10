#include "S25FL512.h"

static const FlashMemoryData_s deviceData = {
    .clockSpiData = 0x00,
    .eraseAllTime = 1000 * 60 * 5,
    .sectorSize = 262144,
    .memorySize = 67108864,
    .pageSize = 512,
};

S25FL512::S25FL512(const uint8_t chipSelectPin)
    : FlashMemoryCommon(chipSelectPin, deviceData) {}
