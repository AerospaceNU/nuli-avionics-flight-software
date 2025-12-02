#ifndef DESKTOP_NULIPACKETUTILS_H
#define DESKTOP_NULIPACKETUTILS_H

#include "message.pb.h"
#include "ReturnCodes.pb.h"

#define NULI_PACKET_START_FLAG 0x7E
#define NULI_PACKET_STOP_FLAG 0x7F
#define NULI_PACKET_CONTROL_FLAG 0x7D
#define NULI_PACKET_ESCAPE 0x20

#define START_FLAG_BYTES 1
#define NULI_MESSAGE_BYTES NULIMessage_size
#define CRC_BYTES 2
#define STOP_FLAG_BYTES 1

#define NULIPacketMaxSize (START_FLAG_BYTES + NULI_MESSAGE_BYTES + CRC_BYTES + STOP_FLAG_BYTES)

/**
 * NULI Packet Definition
 * [START BYTE][...PAYLOAD...][CRC][END BYTE]
 *
 * Questions:
 * Should this be a utility class? Should methods be static?
 * In favor of static: https://softwareengineering.stackexchange.com/questions/352624/static-vs-non-static-in-embedded-systems
 * Against static:
 *
 * Encoding Pipeline
 *  encode NULIMessage Payload --> Calculate CRC --> Add Byte Stuffing
 * Decoding Pipeline
 *  Remove Byte Stuffing --> Validate CRC --> decode NULIMessage Payload
 */
class NULIPacketUtils {
public:
    ReturnCode encode(NULIMessage *nuliMessage, uint8_t *dst_buffer, uint32_t *bytes_written);

    ReturnCode decode(uint8_t *buffer, uint32_t length, NULIMessage *message);

private:
    uint32_t encodeNULIMessagePayload(NULIMessage *nuliMessage, uint8_t *buffer);

    ReturnCode decodeNULIMessagePayload(NULIMessage *nuliMessage, uint8_t *payload_buffer, uint32_t payload_length);

    uint32_t addByteStuffing(const uint8_t *src, uint32_t src_length, uint8_t* dst, uint32_t dst_max_length);

    uint32_t removeByteStuffing(const uint8_t *src, uint32_t src_length, uint8_t *dst);

    uint16_t calculateCRC(const uint8_t* buffer, uint32_t buffer_size);

    static const uint32_t m_buffer_size = NULIPacketMaxSize * 2; // TODO: Change to something
    uint8_t m_encoding_buffer[m_buffer_size];
    uint8_t m_decoding_buffer[m_buffer_size];
};


#endif //DESKTOP_NULIPACKETUTILS_H
