#include "MX25L256.h"

static const FlashMemoryData_s deviceData = {
    // 8 min covers the Winbond W25Q256JVFIQ's rated 400s chip-erase max too (some SillyGoose V2
    // boards use it instead - same page/sector/memory size and commands, per its datasheet).
    .eraseAllTime = 1000 * 60 * 8,
    .sectorSize = 262144 / 4,
    .memorySize = 33554432,
    .pageSize = 256,
};

MX25L256::MX25L256(const uint8_t chipSelectPin)
    : FlashMemoryCommon(chipSelectPin, deviceData) {}
