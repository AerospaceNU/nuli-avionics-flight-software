#include "Avionics.h"
#include <Arduino.h>
#include "pinmaps/SillyGoosePinmap.h"
#include "drivers/arduino/ArduinoAvionicsHelper.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/ArduinoFram.h"
#include "drivers/arduino/ArduinoSerialReader.h"
#include "core/HardwareAbstraction.h"
#include "core/Configuration.h"
#include "core/ConfigurationCliBinding.h"
#include "core/BasicLogger.h"
#include "core/cli/IntegratedParser.h"

bool testCrcCheck = false;
bool validCheckCheck = false;


// Hardware
ArduinoSystemClock arduinoClock;
SerialDebug serialDebug(AVIONICS_ARGUMENT_isDev); // Only wait for serial connection if in dev mode
ArduinoFram fram(FRAM_CS_PIN);
HardwareAbstraction hardware;
ArduinoSerialReader<500> serialReader(true);
IntegratedParser cliParser;

ArgumentFlag<const char *> value("--str", "hi", "str", true, 255, []() {
    Serial.println(value.getValueDerived());
});
BaseFlag *flagGroup[] = {&value};

// Configuration

uint8_t buffer[1000];       // Allow for long configs
ConfigurationID_t sillyGooseRequiredConfigs[] = {   // No RADIO_FREQUENCY_c to have one invalid config
        FLIGHT_STATE_c, GROUND_ELEVATION_c, GROUND_TEMPERATURE_c, BOARD_ORIENTATION_c, MAIN_ELEVATION_c, DROGUE_DELAY_c, PYRO_FIRE_DURATION_c, BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c,
        TEST_STRUCT_c, TEST_STRING_c
    };
Configuration configuration({sillyGooseRequiredConfigs, Configuration::REQUIRED_CONFIGS}, buffer);

// CLI -> configuration bindings. Generates a CLI command to get/set configuration value.
ConfigurationCliBindings<FLIGHT_STATE_c,
                         GROUND_ELEVATION_c,
                         GROUND_TEMPERATURE_c,
                         BOARD_ORIENTATION_c,
                         RADIO_FREQUENCY_c,
                         MAIN_ELEVATION_c,
                         DROGUE_DELAY_c,
                         PYRO_FIRE_DURATION_c,
                         BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c,
                         TEST_STRUCT_c,
                         TEST_STRING_c,
                         CONFIGURATION_VERSION_c> configurationCliBindings;

void setup() {
    // Initialize
    disableChipSelectPins({FRAM_CS_PIN, FLASH_CS_PIN}); // All CS pins must disable prior to SPI device setup on multi device buses to prevent one device from locking the bus
    configuration.setDefault<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>(VOLTAGE_SENSE_SCALE); // Configuration defaults MUST be called prior to configuration.setup() for it to have effect

    // Setup Hardware
    const int16_t framID = hardware.appendFramMemory(&fram);
    hardware.setup(&serialDebug, &arduinoClock, 100);

    if (testCrcCheck) {
        uint8_t value;
        fram.read(10, &value, 1);
        value ^= (1 << 5);   // Flip the 5th bit (bit index 5, zero-based)
        fram.write(10, &value, 1);
    }

    // Setup components
    serialDebug.message("SETTING UP COMPONENTS");
    configuration.setup(&hardware, framID); // Must be called first, for everything else to be able to use the configuration
    configurationCliBindings.setupAll(&configuration, &cliParser, &serialDebug);
    cliParser.setup(&serialReader, &serialDebug);
    cliParser.addFlagGroup(flagGroup);
    serialDebug.message("COMPONENTS SET UP COMPLETE\r\n");


    // serialDebug.message("Null config (this should be 0) : %d", configuration.getConfigurable<RADIO_FREQUENCY_c>().isValid());

    if (validCheckCheck) {
        serialDebug.message("Valid check : %.2f", configuration.getConfigurable<GROUND_ELEVATION_c>().get());
        configuration.getConfigurable<GROUND_ELEVATION_c>().set(-501);
        serialDebug.message("Valid check : %.2f", configuration.getConfigurable<GROUND_ELEVATION_c>().get());
        configuration.getConfigurable<GROUND_ELEVATION_c>().forceSet(-501);
        serialDebug.message("Valid check : %.2f", configuration.getConfigurable<GROUND_ELEVATION_c>().get());
    }

    Serial.println(configuration.getConfigurable<TEST_STRING_c>().get().str);
}

void loop() {
    cliParser.runCli();
    configuration.pushUpdatesToMemory();
}
