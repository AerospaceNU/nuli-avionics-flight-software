// One-off ground station for the NULI Radio Board (SAMD51J19A + SX1262, variants/radio_board).
// Behaves like a SeriousGoose in GROUND_STATION_MODE, so SillyGooseTool treats it as one.
// The board has no FRAM, GPS or sensors: config is kept in the MCU's own flash and the
// GPS lines report no fix.
#include "Arduino.h"
#include "Avionics.h"
#include "util/Timer.h"
#include "drivers/arduino/ArduinoAvionicsHelper.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/SX1262Radio.h"
#include "drivers/arduino/ArduinoWatchdog.h"
#include "drivers/samd51/Samd51InternalFlashMemory.h"
#include "core/HardwareAbstraction.h"
#include "core/GroundStationRelay.h"
#include "builds/SeriousGooseLogData.h"
#include "core/configuration/Configuration.h"
#include "core/configuration/ConfigurationCliBinding.h"
#include "core/cli/SimpleFlag.h"
#include "core/cli/IntegratedParser.h"

constexpr uint32_t RX_LED_ON_MS = 100;

// Hardware
ArduinoSystemClock arduinoClock;
ArduinoWatchdog watchdog; // must precede serialDebug, which pets it during its dev-mode wait
SerialDebug<500> serialDebug(AVIONICS_ARGUMENT_isDev, &watchdog);
// TCXO runs off 3V3 (not DIO3) and DIO2 drives the RF switch
SX1262Radio radio(PIN_RADIO_NSS, PIN_RADIO_DIO1, PIN_RADIO_RESET, PIN_RADIO_BUSY, SX1262Radio::NO_PIN, SX1262Radio::NO_PIN, 915.0f, 0.0f);
Samd51InternalFlashMemory<MAX_CONFIGURATION_LENGTH * 2> configMemory;
GPS noGps; // never gets a fix

// Core components
HardwareAbstraction hardware(serialDebug, arduinoClock, watchdog, 100);
GroundStationRelay groundStationRelay;
IntegratedParser cliParser;
Alarm rxLedTimer;
// Configuration
ConfigurationID_t radioBoardRequiredConfigs[] = {FIRMWARE_VERSION_c, BOARD_NAME_c, RADIO_FREQUENCY_c, LORA_SPREADING_FACTOR_c};
Configuration configuration({
        radioBoardRequiredConfigs,
        Configuration::REQUIRED_CONFIGS
    });
ConfigurationData<float> radioFrequency;
ConfigurationData<int32_t> spreadingFactor;
float appliedFrequency = 0;
int32_t appliedSpreadingFactor = 0;
ConfigurationCliBindings<FIRMWARE_VERSION_c,
                         BOARD_NAME_c,
                         RADIO_FREQUENCY_c,
                         LORA_SPREADING_FACTOR_c,
                         CONFIGURATION_VERSION_c> configurationCliBindings;
// CLI
SimpleFlag resetBoard("--reset", "Reboots the board", true, [](DebugStream*) { NVIC_SystemReset(); });
SimpleFlag helpFlag("--help", "Prints all commands", true, [](DebugStream* debugStream) { cliParser.printHelp(debugStream); });
SimpleFlag logFormatFlag("--logFormat", "Prints the relayed SeriousGoose log data struct definition", true, [](DebugStream* debugStream) {
    static const char prefix[] = "LOG_FORMAT\t";
    debugStream->writeRaw(prefix, sizeof(prefix) - 1);
    debugStream->writeRaw(HEADER_OUTPUT, strlen(HEADER_OUTPUT));
    debugStream->writeRaw("\n", 1);
});
BaseFlag* resetBoardGroup[] = {&resetBoard};
BaseFlag* helpGroup[] = {&helpFlag};
BaseFlag* logFormatGroup[] = {&logFormatFlag};

void setup() {
    watchdog.disable(); // SAMD's WDT survives NVIC_SystemReset()
    watchdog.enable(4000);
    disableChipSelectPins({PIN_RADIO_NSS});
    pinMode(PIN_LED, OUTPUT);
    configuration.setDefault<BOARD_NAME_c>("SeriousGooseGround (Radio Board)");

    hardware.setup();
    int16_t configMemoryID = hardware.appendFramMemory(&configMemory);
    hardware.appendRadioLink(&radio);

    serialDebug.message("SETTING UP COMPONENTS");
    configuration.setup(&hardware, configMemoryID);
    configurationCliBindings.setupAll(&configuration, &cliParser);
    cliParser.addFlagGroup(resetBoardGroup);
    cliParser.addFlagGroup(helpGroup);
    cliParser.addFlagGroup(logFormatGroup);
    cliParser.addStream(&serialDebug);
    groundStationRelay.setup(&cliParser, &radio, &noGps);
    radioFrequency = configuration.getConfigurable<RADIO_FREQUENCY_c>();
    spreadingFactor = configuration.getConfigurable<LORA_SPREADING_FACTOR_c>();
    appliedFrequency = radioFrequency.get();
    appliedSpreadingFactor = spreadingFactor.get();
    radio.setFrequency(appliedFrequency);
    radio.setSpreadingFactor(appliedSpreadingFactor);
    serialDebug.message("COMPONENTS SET UP COMPLETE\r\n");
    watchdog.enable(100);
    watchdog.pet();
}

void loop() {
    const Timestamp_s timestamp = hardware.enforceLoopTime();
    hardware.runAndReadAllHardware();

    cliParser.runCli();
    configuration.pushUpdatesToMemory();

    // Radio settings set over the CLI apply right away, no reboot needed.
    // Retried each tick: the radio rejects changes while transmitting.
    if (radioFrequency.get() != appliedFrequency && radio.setFrequency(radioFrequency.get())) {
        appliedFrequency = radioFrequency.get();
    }
    if (spreadingFactor.get() != appliedSpreadingFactor && radio.setSpreadingFactor(spreadingFactor.get())) {
        appliedSpreadingFactor = spreadingFactor.get();
    }

    if (radio.isMessageAvailable()) {
        digitalWrite(PIN_LED, HIGH);
        rxLedTimer.startAlarm(timestamp.runtime_ms, RX_LED_ON_MS);
    } else if (rxLedTimer.isAlarmFinished(timestamp.runtime_ms)) {
        digitalWrite(PIN_LED, LOW);
    }

    groundStationRelay.tick(timestamp, &serialDebug);
}
