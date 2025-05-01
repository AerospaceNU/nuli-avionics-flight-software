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

struct __attribute__((packed)) GPSPacket {
    float altitude;
    float HDOP;
    uint8_t fixQuality;
    float latitude;
    float longitude;
    uint8_t satellitesTracked;
};




#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_RADIOPACKETDEFINITIONS_H
