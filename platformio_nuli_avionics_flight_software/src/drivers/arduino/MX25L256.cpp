#include "MX25L256.h"

static const FlashMemoryData_s deviceData = {
    .eraseAllTime = 1000 * 60 * 5,
    .sectorSize = 262144 / 4,
    .memorySize = 33554432,
    .pageSize = 256,
};

MX25L256::MX25L256(const uint8_t chipSelectPin)
    : FlashMemoryCommon(chipSelectPin, deviceData) {}
