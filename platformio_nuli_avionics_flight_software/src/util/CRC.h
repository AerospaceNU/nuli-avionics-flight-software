#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include <stddef.h>

// ------------------- CRC-8 -------------------
static inline uint8_t crc8(const void* data, size_t len) {
    const uint8_t* data2 = (uint8_t*)data;
    uint8_t crc = 0x00; // common init for CRC-8
    while (len--) {
        crc ^= *data2++;
        for (uint8_t i = 0; i < 8; i++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1); // polynomial 0x07
    }
    return crc;
}

// ------------------- CRC-16 (CCITT) -------------------
static inline uint16_t crc16(const void* data, size_t len) {
    const uint8_t* data2 = (uint8_t*)data;
    uint16_t crc = 0xFFFF; // initial value
    while (len--) {
        crc ^= (uint16_t)(*data2++) << 8;
        for (uint8_t i = 0; i < 8; i++)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1); // polynomial 0x1021
    }
    return crc;
}

// ------------------- CRC-32 (IEEE 802.3) -------------------
static inline uint32_t crc32(const void* data, size_t len) {
    const uint8_t* data2 = (uint8_t*)data;
    uint32_t crc = 0xFFFFFFFF; // initial value
    while (len--) {
        crc ^= *data2++;
        for (uint8_t i = 0; i < 8; i++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : (crc >> 1); // polynomial 0x04C11DB7 reversed
    }
    return ~crc;
}

#endif //CRC_H
