Usage from `proto` directory: \
`python3 ../../lib/nanopb/generator/nanopb_generator.py -D ./c/ message.proto nanopb.proto`

The -D sets the output directory \
The final parts sets the proto messages to compile

A message with a uint64, uint32, and a float should theoretically be 16 
bytes. Due to Protobuf's encoding, we are able to reduce that down to 
14 bytes for the code snippet in `ProtobufDesktopTest.cpp`.

After compilation, PlatformIO reports Flash memory usage of 4.3KB for
pb_decode.c and 2.4KB for pb_encode.c. 