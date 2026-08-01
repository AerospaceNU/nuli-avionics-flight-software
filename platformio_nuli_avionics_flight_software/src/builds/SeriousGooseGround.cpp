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

// Half-duplex, shared channel with the flight computer's downlink - --send queues its payload and
// fires at a predicted safe window (learned cadence), not immediately/reactively, so bad downlink rx can't block uplink.
static char s_pendingSendBuffer[256];
static uint32_t s_pendingSendLength = 0; // 0 = nothing queued
// Anchor (last confirmed downlink) + cadence predict future downlink times - resynced each rx, but sends don't wait on one.
static uint32_t s_lastDownlinkRxTimeMs = 0; // 0 = no downlink received yet this session
static uint32_t s_downlinkCadenceMs = 0; // 0 = not yet learned (need at least one interval)

// Only fails if the radio is already transmitting - left queued to retry next tick.
void trySendPending() {
    if (s_pendingSendLength == 0) return;
    if (radio.startTransmit(s_pendingSendBuffer, s_pendingSendLength)) {
        s_pendingSendLength = 0;
    }
}

// Anchor is set when a downlink finishes decoding, i.e. right as the flight computer's TX ends - so
// the danger zone is the LATTER part of the cycle (approaching its next TX), not the first half.
// ~543ms on-air of 1000ms leaves ~457ms safe; 35% margin covers clock drift, not tuned precisely.
static constexpr uint32_t SAFE_WINDOW_FRACTION_PERCENT = 35;
bool inSafeUplinkWindow(uint32_t nowMs) {
    if (s_downlinkCadenceMs == 0) return true; // nothing learned yet to predict from - don't block on it
    const uint32_t cyclePos = (nowMs - s_lastDownlinkRxTimeMs) % s_downlinkCadenceMs;
    return cyclePos < (s_downlinkCadenceMs * SAFE_WINDOW_FRACTION_PERCENT) / 100;
}

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
ArgumentFlag<const char*> sendFlag("--send", "Queue a string payload to send up to the rocket over radio - transmitted at the next predicted safe window (based on the learned downlink cadence) rather than immediately, to avoid colliding with the flight computer's own transmit", true, 255, []() {
    const char* payload = sendFlag.getValueDerived();
    // +1 for the null terminator; truncated safely if payload ever exceeded the buffer (CLI input never will).
    uint32_t len = (uint32_t)strlen(payload) + 1;
    if (len > sizeof(s_pendingSendBuffer)) len = sizeof(s_pendingSendBuffer);
    memcpy(s_pendingSendBuffer, payload, len);
    s_pendingSendBuffer[sizeof(s_pendingSendBuffer) - 1] = '\0';
    s_pendingSendLength = len;
    serialDebug.message("Uplink queued (%lu bytes) - sending at the next predicted safe window", (unsigned long)len);
});
SimpleFlag resetBoard("--reset", "Reset the board", true, 255, []() { NVIC_SystemReset(); });
BaseFlag* sendGroup[] = {&sendFlag};
BaseFlag* resetBoardGroup[] = {&resetBoard};

void setup() {
    disableChipSelectPins({FRAM_CS_PIN}); // Must disable prior to SPI device setup on multi-device buses to prevent one device from locking the bus
    configuration.setDefault<BOARD_NAME_c>(GROUND_STATION_NAME); // Configuration defaults MUST be called prior to configuration.setup() for it to have effect

    // Setup Hardware
    int16_t framID = hardware.appendFramMemory(&fram);
    // Radio before GPS - GPS negotiation can block a while, and the radio matters more.
    hardware.appendRadioLink(&radio);
    hardware.appendGPS(&gps);
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

        if (s_lastDownlinkRxTimeMs != 0) {
            const uint32_t rawInterval = timestamp.runtime_ms - s_lastDownlinkRxTimeMs;
            uint32_t impliedCadence = rawInterval;
            if (s_downlinkCadenceMs > 0) {
                // ~3x the last cadence means 2 packets were dropped, not that cadence tripled - round to nearest multiple so jitter near a boundary doesn't misclassify.
                uint32_t multiple = (rawInterval + s_downlinkCadenceMs / 2) / s_downlinkCadenceMs;
                if (multiple == 0) multiple = 1;
                impliedCadence = rawInterval / multiple;
            }
            // IIR smoothing (3:1) so one packet's jitter doesn't jerk the predicted schedule.
            s_downlinkCadenceMs = (s_downlinkCadenceMs == 0) ? impliedCadence : (s_downlinkCadenceMs * 3 + impliedCadence) / 4;
        }
        s_lastDownlinkRxTimeMs = timestamp.runtime_ms;
        // Send before the USB print below - the safe window is finite (~457ms) and a slow host could eat into it.
        trySendPending();

        char hexBuffer[sizeof(message.data) * 2 + 1];
        bytesToHex(message.data, message.length, hexBuffer);
        serialDebug.data("RADIO_RX\t%d\t%.2f\t%s", message.rssi, message.snr, hexBuffer);
    }

    // Elapsed-time check, not gated on hearing a downlink - keeps firing through lost downlink packets.
    if (s_pendingSendLength > 0 && inSafeUplinkWindow(timestamp.runtime_ms)) {
        trySendPending();
    }
}
