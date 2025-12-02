#include "NULIPacketUtils.h"

// nanopb files
#include "pb_encode.h"
#include "pb_decode.h"

// Public
ReturnCode NULIPacketUtils::encode(NULIMessage *nuliMessage, uint8_t *dst_buffer, uint32_t *bytes_written) {
    // clear encoding buffer
    memset(m_encoding_buffer, 0, m_buffer_size);

    // encode NULIMessage payload
    uint32_t encode_bytes_written = encodeNULIMessagePayload(nuliMessage, m_encoding_buffer);
    if (encode_bytes_written == 0) {
        return ReturnCode_NULIPACKET_ENCODING_ERROR;
    }

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
    if (stuffed_len == 0) {
        return ReturnCode_NULIPACKET_ENCODING_ERROR;
    }

    dst_buffer[0] = NULI_PACKET_START_FLAG;
    dst_buffer[START_FLAG_BYTES + stuffed_len] = NULI_PACKET_STOP_FLAG;

    *bytes_written = stuffed_len + START_FLAG_BYTES + STOP_FLAG_BYTES;

    return ReturnCode_SUCCESS;
}

// the payload should be the entire packet [START_FLAG][...PAYLOAD...][CRC][STOP_FLAG]
// the radio manager should have already stripped the start and end bytes
ReturnCode NULIPacketUtils::decode(uint8_t *buffer, uint32_t length, NULIMessage *message) {
    // clear decoding buffer
    memset(m_decoding_buffer, 0, m_buffer_size);

    // Check start and stop flags
    if (buffer[0] != NULI_PACKET_START_FLAG) {
        return ReturnCode_NULIPACKET_INVALID_START_FLAG;
    }

    if (buffer[length - 1] != NULI_PACKET_STOP_FLAG) {
        return ReturnCode_NULIPACKET_INVALID_STOP_FLAG;
    }

    uint32_t stuffed_payload_length = length - START_FLAG_BYTES - STOP_FLAG_BYTES;

    // remove byte stuffing
    uint32_t bytes_written = removeByteStuffing(buffer + START_FLAG_BYTES, stuffed_payload_length, m_decoding_buffer);

    if (bytes_written == 0) {
        return ReturnCode_NULIPACKET_DECODING_ERROR;
    }

    // check CRC
    uint32_t payload_len_without_crc = bytes_written - CRC_BYTES;
    uint16_t received_crc = m_decoding_buffer[payload_len_without_crc] | (m_decoding_buffer[payload_len_without_crc + 1] << 8);

    uint16_t actual_crc = calculateCRC(m_decoding_buffer, payload_len_without_crc);

    if (received_crc != actual_crc) {
        return ReturnCode_NULIPACKET_INVALID_CRC;
    }

    // decode NULIMessage payload
    ReturnCode return_code = decodeNULIMessagePayload(message, m_decoding_buffer, payload_len_without_crc);
    if (return_code != ReturnCode_SUCCESS) {
        return return_code;
    }

    return ReturnCode_SUCCESS;
}

// Private
uint32_t NULIPacketUtils::encodeNULIMessagePayload(NULIMessage *nuliMessage, uint8_t *dst_buffer) { // NOLINT(*-convert-member-functions-to-static)
    // encode protobuf packet
    pb_ostream_t stream = pb_ostream_from_buffer(dst_buffer, m_buffer_size);
    bool result = pb_encode(&stream, NULIMessage_fields, nuliMessage);

    if (!result) {
        return 0;
    }

    return stream.bytes_written;
}

ReturnCode NULIPacketUtils::decodeNULIMessagePayload(NULIMessage *nuliMessage, uint8_t *payload_buffer, uint32_t payload_length) { // NOLINT(*-convert-member-functions-to-static)
    memset(nuliMessage, 0, sizeof(NULIMessage));
    pb_istream_t istream = pb_istream_from_buffer(payload_buffer, payload_length);
    bool status = pb_decode(&istream, NULIMessage_fields, nuliMessage);

    if (!status) {
        return ReturnCode_NULIPACKET_NANOPB_DECODE_ERROR;
    }

    return ReturnCode_SUCCESS;
}

uint32_t NULIPacketUtils::addByteStuffing(const uint8_t *src, uint32_t src_length, uint8_t *dst, uint32_t dst_max_length) {  // NOLINT(*-convert-member-functions-to-static)
    uint32_t dst_index = 0;

    for (uint32_t src_index = 0; src_index < src_length && dst_index < dst_max_length; src_index++) {
        if (dst_index >= dst_max_length) return 0;
        // if NULI_PACKET_START_FLAG or NULI_PACKET_STOP_FLAG or
        // NULI_PACKET_ESCAPE_FLAG is present escape it
        if (src[src_index] == NULI_PACKET_START_FLAG ||
            src[src_index] == NULI_PACKET_CONTROL_FLAG ||
            src[src_index] == NULI_PACKET_STOP_FLAG) {

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
                return 0;
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
