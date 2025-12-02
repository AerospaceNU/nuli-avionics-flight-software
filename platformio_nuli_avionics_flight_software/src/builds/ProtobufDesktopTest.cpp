#include <cstdio>
#include "NULIPacketUtils.h"
#include "message.pb.h"

// nanopb files
#include "pb_encode.h"
#include "pb_decode.h"
#include "pb_common.h"

#define BUFFER_SIZE 256

class ProtobufHandler {
public:

protected:
private:
    uint8_t m_encode_buffer[BUFFER_SIZE];
    size_t m_encoded_size;
};


//int main() {
//    uint8_t buffer[100] = {0};
//    uint16_t bytes_written;
//
//    {
//        NULIMessage message = NULIMessage_init_zero;
//        message.has_timestamp_ms = true;
//        message.timestamp_ms = 123456789;
//        message.has_message_id = true;
//        message.message_id = 1;
//
//        message.which_payload = NULIMessage_command_tag;
//        message.payload.command.has_deploymentAltitude = true;
//        message.payload.command.deploymentAltitude = 2.3;
//
//        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
//        bool status = pb_encode(&stream, NULIMessage_fields, &message);
//
//        if (!status)
//        {
//            printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
//            return 1;
//        }
//        bytes_written = stream.bytes_written;
//    }
//
//    printf("Bytes written is: %d\n", bytes_written);
//
//    // we can now send the stream over the network
//    {
//        NULIMessage message = NULIMessage_init_zero;
//        pb_istream_t stream = pb_istream_from_buffer(buffer, bytes_written);
//        bool status = pb_decode(&stream, NULIMessage_fields, &message);
//
//        if (!status) {
//            printf("decoding failed");
//            return -1;
//        }
//        printf("Message was %d\n", message.message_id);
//    }
//}

NULIPacketUtils nuliPacketUtils;

int main() {
    uint8_t dst_buffer[100];
    uint32_t bytes_written = 0;

    // encode
    {
        NULIMessage message = NULIMessage_init_zero;
        message.has_timestamp_ms = true;
        message.timestamp_ms = 23;
        message.has_message_id = true;
        message.message_id = 126;

        message.which_payload = NULIMessage_command_tag;
        message.payload.command.has_deploymentAltitude = true;
        message.payload.command.deploymentAltitude = 2.3;

        int return_code = nuliPacketUtils.encode(&message, dst_buffer, &bytes_written);

        if (return_code < 0) {
            printf("Encoding failed with code: %d", return_code);
            return -1;
        }
    }

    // decode
    {
        NULIMessage message = NULIMessage_init_zero;
        int return_code = nuliPacketUtils.decode(dst_buffer, bytes_written, &message);

        if (return_code < 0) {
            printf("Decoding failed with code: %d", return_code);
        } else {
            printf("Timestamp: %llu\n"
                   "Message Id: %d\n"
                   "Payload Deployment Altitude: %f\n",
                   message.timestamp_ms,
                   message.message_id,
                   message.payload.command.deploymentAltitude);
        }
    }
}