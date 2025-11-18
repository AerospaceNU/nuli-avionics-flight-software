#include <Arduino.h>
#include "Avionics.h"
#include <SPI.h>
#include <Wire.h>
#include <cstdio>

// Custon Proto files
#include "message.pb.h"

// nanopb files
#include "nanopb/pb_encode.h"
#include "nanopb/pb_decode.h"
#include "nanopb/pb_common.h"

int test() {
    uint8_t buffer[100] = {0};
    uint16_t bytes_written;

    {
        NULIMessage message = NULIMessage_init_zero;
        message.has_timestamp_ms = true;
        message.timestamp_ms = 123456789;
        message.has_message_id = true;
        message.message_id = 1;

        message.which_payload = NULIMessage_command_tag;
        message.payload.command.has_deploymentAltitude = true;
        message.payload.command.deploymentAltitude = 2.3;

        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        bool status = pb_encode(&stream, NULIMessage_fields, &message);

        if (!status)
        {
            printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
            return 1;
        }
        bytes_written = stream.bytes_written;
    }

    // we can now send the stream over the network
    {
        NULIMessage message = NULIMessage_init_zero;
        pb_istream_t stream = pb_istream_from_buffer(buffer, bytes_written);
        bool status = pb_decode(&stream, NULIMessage_fields, &message);

        if (!status) {
            printf("decoding failed");
            return -1;
        }
        printf("Message was %lu\n", message.message_id);
    }

    return 0;
}

void setup() {
    test();
}

void loop() {
    Serial.println("Hello!");
}