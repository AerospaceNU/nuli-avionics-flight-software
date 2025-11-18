#ifndef DESKTOP_NULIPACKET_H
#define DESKTOP_NULIPACKET_H

#include "message.pb.h"

#define NULI_PACKET_START_FLAG 0x7E
#define NULI_PACKET_END_FLAG 0x7F
#define NULI_PACKET_CONTROL_FLAG 0x7D
#define NULI_PACKET_ESCAPE 0x20

#define START_FLAG_BYTES 1
#define NULI_MESSAGE_BYTES NULIMessage_size
#define CRC_BYTES 2
#define END_FLAG_BYTES 1

#define NULIPacketMaxSize (START_FLAG_BYTES + NULI_MESSAGE_BYTES + CRC_BYTES + END_FLAG_BYTES)

/**
 * NULI Packet Definition
 * [START BYTE][...PAYLOAD...][CRC][END BYTE]
 *
 * Questions:
 * Should this be a utility class? Should methods be static?
 * In favor of static: https://softwareengineering.stackexchange.com/questions/352624/static-vs-non-static-in-embedded-systems
 * Against static:
 */
class NULIPacket {
public:
    int encode(NULIMessage &nuliMessage, uint8_t *dst_buffer, uint32_t *bytes_written);

    int decode(uint8_t &buffer, uint32_t length, NULIMessage &message);

private:
    static int encodeNULIMessagePayload(NULIMessage &nuliMessage, uint8_t *buffer, uint32_t *bytes_written);

    int decodeNULIMessagePayload();

    int addByteStuffing(const uint8_t *src, uint32_t src_length, uint8_t* dst, uint32_t dst_max_length);

    int removeByteStuffing(const uint8_t *src, uint32_t src_length, uint8_t *dst, uint32_t dst_max_length);

    int calculateCRC();

    int validateCRC();
};


#endif //DESKTOP_NULIPACKET_H
