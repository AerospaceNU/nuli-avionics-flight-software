#include "MX25L256.h"

static const FlashMemoryData_s deviceData = {
    // 8 min also covers the W25Q256JVFIQ (400 s chip-erase max) fitted on some SillyGoose V2 boards
    .eraseAllTime = 1000 * 60 * 8,
    .sectorSize = 262144 / 4,
    .memorySize = 33554432,
    .pageSize = 256,
};

MX25L256::MX25L256(const uint8_t chipSelectPin)
    : FlashMemoryCommon(chipSelectPin, deviceData) {}
