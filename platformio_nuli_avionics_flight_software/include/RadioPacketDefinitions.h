#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RADIOPACKETDEFINITIONS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RADIOPACKETDEFINITIONS_H

#include <cstdint>

#define MAX_PACKET_DATA_SIZE 32

struct __attribute__((packed)) RadioPacketHeader {
    uint8_t packetNumber;
    uint32_t packetLength;
    uint8_t groupId;
    uint8_t flagId;
};



struct __attribute__((packed)) HeaderPacket {
  uint8_t startByte;        //
  uint8_t flagGroupUid;     //
  uint8_t flagUid;          //
  uint8_t packetLength;     //
  uint8_t packetNumber;     //
  uint8_t crc;              //
};

typedef struct __attribute__((packed)) {
  HeaderPacket header;      // 6 bytes
  uint16_t data;            // 2 bytes
} RadioPacket;

struct __attribute__((packed)) CliPacket { };

struct __attribute__((packed)) : CliPacket {

} ConfigPacket;




#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RADIOPACKETDEFINITIONS_H
