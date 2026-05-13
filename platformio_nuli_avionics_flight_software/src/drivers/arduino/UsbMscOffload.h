#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USB_MSC_OFFLOAD_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USB_MSC_OFFLOAD_H

#include <Avionics.h>
#include "core/generic_hardware/FlashMemory.h"
#include "core/generic_hardware/DebugStream.h"

// Exposes a virtual FAT16 USB Mass Storage volume that streams log data out of
// flash as TSV files, with one FLIGHTNN.TSV per boot recorded on the chip.
// Plus a placeholder CONFIG.TXT (round-trip works -- not yet wired to the real
// Configuration system).
//
// TinyUSB callbacks must be raw C functions, so the implementation keeps a
// single file-static instance in UsbMscOffload.cpp and dispatches through it.
// Only one usable instance per build.
//
// Flash format expected (matches BasicLogger):
//   - Contiguous fixed-size entries from offset 0.
//   - Byte 0 of each entry is the id; 0xFF marks empty / end-of-log.
// Flights are divided by the boot marker BasicLogger writes from setup():
//   id == 0x03 and the message text starts with "Logger setup".
class UsbMscOffload {
public:
    // Formats a single log entry into a textual row. The formatter writes via
    // the provided DebugStream (typically a single `debug->data("...")` call).
    // The class captures those bytes, strips the trailing newline, pads to a
    // fixed row width, and appends CRLF. Branch on entry[0] (the id byte) to
    // handle data vs. message vs. marker rows.
    //
    // The DebugStream contract is used (rather than snprintf into a buffer)
    // because newlib-nano on SAMD21 -- the default printf -- silently drops
    // %f, so anything that goes through snprintf comes out without its floats.
    // The existing DebugStream::data() path has its own %f implementation that
    // works, and is what the CLI offload already uses.
    using RowFormatterFn = void (*)(const uint8_t* entry, DebugStream* debug);

    // flash      flash chip to scan and stream from
    // entrySize  size of one log entry in bytes (id byte + packed data)
    // tsvHeader  header text emitted as row 0 of every TSV file
    // formatter  callback to format one entry into a row
    UsbMscOffload(FlashMemory* flash, uint16_t entrySize,
                  const char* tsvHeader, RowFormatterFn formatter);

    // Scans flash, builds the file table, configures USB MSC, and turns the
    // volume on. Call once from setup() after the flash chip is initialized.
    void begin();
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USB_MSC_OFFLOAD_H
