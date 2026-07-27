#include "Arduino.h"
#include "Avionics.h"
#include "pinmaps/SeriousGoosePinmap.h"
#include "util/Timer.h"
#include "drivers/arduino/ArduinoAvionicsHelper.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/UBloxV2.h"
#include "drivers/arduino/SX1262Radio.h"
#include "drivers/arduino/ArduinoFram.h"
#include "drivers/arduino/ArduinoSerialReader.h"
#include "core/HardwareAbstraction.h"
#include "core/configuration/Configuration.h"
#include "core/configuration/ConfigurationCliBinding.h"
#include "core/cli/IntegratedParser.h"
#include "core/cli/ArgumentFlag.h"
#include "core/cli/SimpleFlag.h"
#include <cstring>

// SeriousGooseGround shares the SeriousGoose PCB/pinmap, but runs a minimal firmware that just
// bridges GPS + radio to USB - no flight state estimation, pyros, or flight logging.
#define GROUND_STATION_NAME "SeriousGooseGroundV1"
#define GPS_REPORT_INTERVAL_MS (1000)

// Hex-encodes `length` bytes of `data` into `outHex` (must be at least length*2+1 bytes) - used to
// report raw, possibly-non-printable radio payloads over the text-based debug stream.
void bytesToHex(const uint8_t* data, uint8_t length, char* outHex) {
    static const char digits[] = "0123456789ABCDEF";
    for (uint8_t i = 0; i < length; i++) {
        outHex[i * 2] = digits[data[i] >> 4];
        outHex[i * 2 + 1] = digits[data[i] & 0x0F];
    }
    outHex[length * 2] = '\0';
}

// Hardware
ArduinoSystemClock arduinoClock;
SerialDebug serialDebug(AVIONICS_ARGUMENT_isDev); // Only wait for serial connection if in dev mode
ArduinoFram fram(FRAM_CS_PIN);
UBloxV2 gps(&Serial1);
SX1262Radio radio(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RESET_PIN, RADIO_BUSY_PIN, RADIO_RX_EN_PIN, RADIO_TX_EN_PIN, 915.0f);
Alarm gpsReportTimer;

// Core components
HardwareAbstraction hardware(serialDebug, arduinoClock, 100);
ArduinoSerialReader<500> serialReader(!AVIONICS_ARGUMENT_isSim);
IntegratedParser cliParser;

// Configuration - RADIO_FREQUENCY/LORA_SPREADING_FACTOR must be set to match the flight
// computer's radio for the two to talk to each other. There's no RADIO_TRANSMIT_INTERVAL here
// since uplink is manually triggered via --send rather than sent on a timer.
ConfigurationID_t groundStationRequiredConfigs[] = {FIRMWARE_VERSION_c, RADIO_FREQUENCY_c, LORA_SPREADING_FACTOR_c, BOARD_NAME_c};
Configuration configuration({groundStationRequiredConfigs, Configuration::REQUIRED_CONFIGS});
ConfigurationCliBindings<FIRMWARE_VERSION_c, RADIO_FREQUENCY_c, LORA_SPREADING_FACTOR_c, BOARD_NAME_c, CONFIGURATION_VERSION_c> configurationCliBindings;

// CLI
ArgumentFlag<const char*> sendFlag("--send", "Send a string payload up to the rocket over radio", true, 255, []() {
    const char* payload = sendFlag.getValueDerived();
    radio.startTransmit((void*)payload, (uint32_t)strlen(payload) + 1); // +1 to include the null terminator, matching the downlink's assumed-string convention
});
SimpleFlag resetBoard("--reset", "Reset the board", true, 255, []() { NVIC_SystemReset(); });
BaseFlag* sendGroup[] = {&sendFlag};
BaseFlag* resetBoardGroup[] = {&resetBoard};

void setup() {
    disableChipSelectPins({FRAM_CS_PIN}); // Must disable prior to SPI device setup on multi-device buses to prevent one device from locking the bus
    configuration.setDefault<BOARD_NAME_c>(GROUND_STATION_NAME); // Configuration defaults MUST be called prior to configuration.setup() for it to have effect

    // Setup Hardware
    int16_t framID = hardware.appendFramMemory(&fram);
    hardware.appendGPS(&gps);
    hardware.appendRadioLink(&radio);
    hardware.setup();

    // Setup components
    serialDebug.message("SETTING UP COMPONENTS");
    configuration.setup(&hardware, framID); // Must be called first, for everything else to be able to use the configuration
    configurationCliBindings.setupAll(&configuration, &cliParser, &serialDebug);
    cliParser.addFlagGroup(sendGroup);
    cliParser.addFlagGroup(resetBoardGroup);
    cliParser.setup(&serialReader, &serialDebug);
    // Radio
    radio.setFrequency(configuration.getConfigurable<RADIO_FREQUENCY_c>().get());
    radio.setSpreadingFactor(configuration.getConfigurable<LORA_SPREADING_FACTOR_c>().get());
    gpsReportTimer.startAlarm(0, 0); // Trigger right away
    // All done
    serialDebug.message("COMPONENTS SET UP COMPLETE\r\n");
}

void loop() {
    const Timestamp_s timestamp = hardware.enforceLoopTime();
    hardware.runAndReadAllHardware(); // Reads sensors, runs any background code for every hardware device

    cliParser.runCli();
    configuration.pushUpdatesToMemory();

    // Periodically report the ground station's own GPS position over USB
    if (gpsReportTimer.isAlarmFinished(timestamp.runtime_ms)) {
        const Coordinates_s coords = gps.getCoordinates();
        serialDebug.data("GPS\t%.6f\t%.6f\t%.2f\t%lu\t%u\t%u\t%d\t%d",
            coords.latitudeDeg, coords.longitudeDeg, coords.altitudeM,
            gps.getUnixTimeS(), gps.getHDOP(), gps.getVDOP(), gps.getFixQuality(), gps.getSatellitesTracked());
        gpsReportTimer.startAlarm(timestamp.runtime_ms, GPS_REPORT_INTERVAL_MS);
    }

    // Forward any received radio packet (raw, hex-encoded bytes) over USB - decoding is left to
    // the host tool, which knows the flight computer's log struct layout
    if (radio.isMessageAvailable()) {
        const RadioLink::RadioMessage message = radio.readMessage();
        char hexBuffer[sizeof(message.data) * 2 + 1];
        bytesToHex(message.data, message.length, hexBuffer);
        serialDebug.data("RADIO_RX\t%d\t%.2f\t%s", message.rssi, message.snr, hexBuffer);
    }
}
