//
// Created by chris on 11/12/2025.
//

#include "NULIPacket.h"

// nanopb files
#include "nanopb/pb_encode.h"
#include "nanopb/pb_decode.h"
#include "nanopb/pb_common.h"

// Public
int NULIPacket::encode(NULIMessage &nuliMessage, uint8_t *dst_buffer, uint32_t *bytes_written) {
    return 0;
}


int NULIPacket::decode(uint8_t &buffer, uint32_t length, NULIMessage &message) {
    return 0;
}

// Private
int NULIPacket::encodeNULIMessagePayload(NULIMessage &nuliMessage, uint8_t *dst_buffer, uint32_t *bytes_written) {
    // encode protobuf packet
    pb_ostream_t stream = pb_ostream_from_buffer(dst_buffer, sizeof(dst_buffer));
    bool result = pb_encode(&stream, NULIMessage_fields, &nuliMessage);

    if (!result) {
        return -1;  //@TODO: Make return code
    }

    *bytes_written = stream.bytes_written;

    return 0;
}

int NULIPacket::decodeNULIMessagePayload() {
    return 0;
}

int NULIPacket::addByteStuffing(const uint8_t *src, uint32_t src_length, uint8_t *dst, uint32_t dst_max_length) {
    uint32_t dst_index = 0;

    for (uint32_t src_index = 0; src_index < src_length && dst_index < dst_max_length; src_index++) {
        // if NULI_PACKET_START_FLAG or NULI_PACKET_END_FLAG or
        // NULI_PACKET_ESCAPE_FLAG is present escape it
        if (src[src_index] == NULI_PACKET_START_FLAG ||
            src[src_index] == NULI_PACKET_CONTROL_FLAG ||
            src[src_index] == NULI_PACKET_END_FLAG) {
            dst[dst_index] = NULI_PACKET_CONTROL_FLAG;
            dst_index++;

            dst[dst_index] = src[src_index] ^ NULI_PACKET_ESCAPE;
        }

        dst_index++;
    }

    return 0;
}

int NULIPacket::removeByteStuffing(const uint8_t *src, uint32_t src_length, uint8_t *dst, uint32_t dst_max_length) {

g

    return 0;
}

int NULIPacket::calculateCRC() {
    return 0;
}

int NULIPacket::validateCRC() {
    return 0;
}
