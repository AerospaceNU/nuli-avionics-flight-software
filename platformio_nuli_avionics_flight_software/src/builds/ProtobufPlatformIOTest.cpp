#include <Arduino.h>
#include "Avionics.h"
#include <SPI.h>
#include <Wire.h>
#include <cstdio>

// Custon Proto files
#include "message.pb.h"
#include "NULIPacketUtils.h"

// nanopb files
#include "pb_encode.h"
#include "pb_decode.h"

NULIPacketUtils nuliPacketUtils;

int test() {
    uint8_t dst_buffer[100];
    uint32_t bytes_written = 0;

    // encode
    {
        NULIMessage message;
        message.has_timestamp_ms = true;
        message.timestamp_ms = 23;
        message.has_message_id = true;
        message.message_id = 126;

        message.which_payload = NULIMessage_command_tag;
        message.payload.command.has_deploymentAltitude = true;
        message.payload.command.deploymentAltitude = 2.3;

        int return_code = nuliPacketUtils.encode(&message, dst_buffer, &bytes_written);

        if (return_code < 0) {
            Serial.print("Encoding failed with code: ");
            Serial.println(return_code);
            return -1;
        }
    }

    // decode
    {
        NULIMessage message;
        int return_code = nuliPacketUtils.decode(dst_buffer, bytes_written, &message);

        if (return_code < 0) {
            Serial.print("Decoding failed with code: ");
            Serial.println(return_code);
        } else {
            Serial.print("Timestamp: ");
            Serial.println(message.timestamp_ms);

            Serial.print("Message Id: ");
            Serial.println(message.message_id);

            Serial.print("Payload Deployment Altitude: ");
            Serial.println(message.payload.command.deploymentAltitude);
        }
    }

    return 0;
}

void setup() {
    test();
}

void loop() {
    Serial.println("Hello!");
}

/*
RAM:   [=         ]  10.7% (used 3520 bytes from 32768 bytes)
Flash: [=         ]   6.0% (used 15640 bytes from 262144 bytes)

RAM:   [=         ]  11.1% (used 3632 bytes from 32768 bytes)
Flash: [=         ]  12.5% (used 32668 bytes from 262144 bytes)
 */