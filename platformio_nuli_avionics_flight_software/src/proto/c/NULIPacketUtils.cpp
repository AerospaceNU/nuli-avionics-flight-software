#include "NULIPacketUtils.h"
#include <cstdio>

// nanopb files
#include "nanopb/pb_encode.h"
#include "nanopb/pb_decode.h"

// Public
/*
* NULI Packet Definition
* [START BYTE][...PAYLOAD...][CRC][END BYTE]
*/
int NULIPacketUtils::encode(NULIMessage *nuliMessage, uint8_t *dst_buffer, uint32_t *bytes_written) {
    // clear encoding buffer
    memset(m_encoding_buffer, 0, m_buffer_size);

    // encode NULIMessage payload
    uint32_t encode_bytes_written = encodeNULIMessagePayload(nuliMessage, m_encoding_buffer);

    // calculate crc
    uint16_t crc = calculateCRC(m_encoding_buffer, encode_bytes_written);

    // add crc to buffer, after the payload
    memcpy(m_encoding_buffer + encode_bytes_written, &crc, sizeof(uint16_t));
    // update bytes_written
    *bytes_written = encode_bytes_written + CRC_BYTES;

    // add byte stuffing to final buffer
    // shift by one to account for start flag
    uint32_t stuffed_max_bytes = NULIPacketMaxSize - (START_FLAG_BYTES + CRC_BYTES + STOP_FLAG_BYTES);
    uint32_t stuffed_len = addByteStuffing(m_encoding_buffer, *bytes_written, (dst_buffer + START_FLAG_BYTES), stuffed_max_bytes);

    dst_buffer[0] = NULI_PACKET_START_FLAG;
    dst_buffer[START_FLAG_BYTES + stuffed_len] = NULI_PACKET_STOP_FLAG;

    *bytes_written = stuffed_len + START_FLAG_BYTES + STOP_FLAG_BYTES;

    return 0;
}

// the payload should be the entire packet [START_FLAG][...PAYLOAD...][CRC][STOP_FLAG]
// the radio manager should have already stripped the start and end bytes
int NULIPacketUtils::decode(uint8_t *buffer, uint32_t length, NULIMessage *message) {
    // clear decoding buffer
    memset(m_decoding_buffer, 0, m_buffer_size);

    // remove start and end flag
    uint8_t flag_size = START_FLAG_BYTES + STOP_FLAG_BYTES;
    length -= flag_size;

    // Use a proper temporary buffer
    uint8_t temp_buffer[256];  // Or use a member variable
    memcpy(temp_buffer, buffer + 1, length);  // skip the start flag

    // remove byte stuffing
    uint32_t bytes_written = removeByteStuffing(temp_buffer, length, m_decoding_buffer);

    // check CRC
    uint32_t payload_len_without_crc = bytes_written - CRC_BYTES;
    uint16_t received_crc = m_decoding_buffer[payload_len_without_crc] | (m_decoding_buffer[payload_len_without_crc + 1] << 8);

    uint16_t actual_crc = calculateCRC(m_decoding_buffer, payload_len_without_crc);

    if (received_crc != actual_crc) {
//        printf("received vs actual crc is: 0x%X vs 0x%X\n", received_crc, actual_crc);
        return -1;  // @TODO Turn into return code
    }

    // decode NULIMessage payload
    int return_code = decodeNULIMessagePayload(message, m_decoding_buffer, payload_len_without_crc);
    if (return_code < 0) {
        return -2;
    }

    return 0;
}

// Private
uint32_t NULIPacketUtils::encodeNULIMessagePayload(NULIMessage *nuliMessage, uint8_t *dst_buffer) { // NOLINT(*-convert-member-functions-to-static)
    // encode protobuf packet
    pb_ostream_t stream = pb_ostream_from_buffer(dst_buffer, m_buffer_size);
    bool result = pb_encode(&stream, NULIMessage_fields, nuliMessage);

    if (!result) {
        return 0;  //@TODO: Make return code
    }

    return stream.bytes_written;
}

int NULIPacketUtils::decodeNULIMessagePayload(NULIMessage *nuliMessage, uint8_t *payload_buffer, uint32_t payload_length) { // NOLINT(*-convert-member-functions-to-static)
    *nuliMessage = NULIMessage_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(payload_buffer, payload_length);
    bool status = pb_decode(&istream, NULIMessage_fields, nuliMessage);

    if (!status) {
        return -1;
    }

    return 0;
}

uint32_t NULIPacketUtils::addByteStuffing(const uint8_t *src, uint32_t src_length, uint8_t *dst, uint32_t dst_max_length) {  // NOLINT(*-convert-member-functions-to-static)
    uint32_t dst_index = 0;

    for (uint32_t src_index = 0; src_index < src_length && dst_index < dst_max_length; src_index++) {
        if (dst_index >= dst_max_length) return -1;
        // if NULI_PACKET_START_FLAG or NULI_PACKET_STOP_FLAG or
        // NULI_PACKET_ESCAPE_FLAG is present escape it
        if (src[src_index] == NULI_PACKET_START_FLAG ||
            src[src_index] == NULI_PACKET_CONTROL_FLAG ||
            src[src_index] == NULI_PACKET_STOP_FLAG) {

            printf("ADDING BYTE STUFFING AT INDEX %d\n", src_index);

            dst[dst_index++] = NULI_PACKET_CONTROL_FLAG;
            dst[dst_index++] = src[src_index] ^ NULI_PACKET_ESCAPE;
        } else {
            dst[dst_index++] = src[src_index];
        }
    }

    return dst_index;
}

uint32_t NULIPacketUtils::removeByteStuffing(const uint8_t *src, uint32_t src_length, uint8_t *dst) {   // NOLINT(*-convert-member-functions-to-static)
    uint32_t dst_index = 0;

    for (uint32_t src_index = 0; src_index < src_length; src_index++) {
        if (src[src_index] == NULI_PACKET_CONTROL_FLAG) {
            if (src_index + 1 >= src_length) {
                return -1;
            }

            src_index++;    // skip the control flag
            dst[dst_index++] = src[src_index] ^ NULI_PACKET_ESCAPE;
        } else {
            dst[dst_index++] = src[src_index];
        }
    }

    return dst_index; // returns number of bytes written
}

// @TODO: Change into using a CRC lookup table
uint16_t NULIPacketUtils::calculateCRC(const uint8_t* buffer, uint32_t buffer_size) { // NOLINT(*-convert-member-functions-to-static)
    uint16_t crc = 0xFFFF;      // Initial value

    for (uint32_t i = 0; i < buffer_size; i++)
    {
        crc ^= buffer[i];

        for (uint8_t b = 0; b < 8; b++)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;   // Polynomial
            else
                crc >>= 1;
        }
    }

    return crc;
}
