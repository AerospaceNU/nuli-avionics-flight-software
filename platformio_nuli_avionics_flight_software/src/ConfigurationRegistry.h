#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRY_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRY_H

#include "Avionics.h"

/**
 * Register all config variables
 * This syntax enables the compile time type checking and fully safe configuration system
 * ALL variables used across all boards/builds must be declared here. Any given board/build
 * will only use a subset of these variable
 * To create a new configuration variable, use the following macro:
 *      DEFINE_CONFIGURATION_VARIABLE(NAME, type_t, defaultValue, conditionForValueToBeValid)
 *
 * How to use conditionForValueToBeValid
 *      This will basically be inserted into a function isValid(const type_t& value) { return conditionForValueToBeValid; };
 *      So you can write 'true' if you want to disable valid checks.
 *      Valid checks will not invalidate the entire configuration, just the single varable
 *      A common way to write this would be 'value >= 0 && value <= 10'
 *      If type_t is a struct: you can access it as a struct: 'value.member >= 0 && value.member <= 10'
 *
 * To construct a new configuration you need to pass it all configuration variables that
 * ANY part of your code uses. You do this by passing an 2-dimensional array of IDs.
 * It's ok to have duplicates in this list: it will filter them out. You will allways need
 * Configuration::REQUIRED_CONFIGS in the constructor for the Configuration to work at all.
 *      ConfigurationID_t requiredConfigs[] = {DROGUE_DELAY_c, MAIN_ELEVATION_c};
 *      Configuration configuration({requiredConfigs, Configuration::REQUIRED_CONFIGS, FlightStateDeterminer::REQUIRED_CONFIGS, StateEstimator1D::REQUIRED_CONFIGS});
 *      configuration.setup(&hardware, framID);     // framID is the Index of the FRAM in the hardware list
 *
 * Interacting with an individual configuration variable:
 *      ConfigurationData<int32_t> flightState = configuration.getConfigurable<FLIGHT_STATE_c>();
 *      int32_t flightState = flightState.get();
 *      flightState.set(ASCENT);
 *
 * Call this every loop, it checks for any changed configuration variables and writes the changes to FRAM
 *      configuration.pushUpdatesToMemory();
 *
 * System limitations:
 *      - There is a maximum number of configuration variables allowed per build, and max total bytes of data.
 *        This is defined in Avionics.h with MAX_CONFIGURATION_NUM and MAX_CONFIGURATION_LENGTH
 *      - Currently the system doesn't support any runtime variability of getConfigurable<>(). Use dependency
 *        injection if a class needs to be able to reference different configuration variables in different builds
 *      - There is a maximum number of configuration variables than can possibly be defined, due to template recursion
 *        limits. Probably around 1000.
 */

// clang-format off
DEFINE_CONFIGURATION_VARIABLE(FLIGHT_STATE, int32_t, PRE_FLIGHT, value >= PRE_FLIGHT && value <= UNKNOWN_FLIGHT_STATE)
DEFINE_CONFIGURATION_VARIABLE(GROUND_ELEVATION, float, 0.0, value >= -500.0 && value <= 9000.0)
DEFINE_CONFIGURATION_VARIABLE(GROUND_TEMPERATURE, float, Constants::STANDARD_TEMPERATURE_K, value >= 180.0 && value <= 350.0)
DEFINE_CONFIGURATION_VARIABLE(BOARD_ORIENTATION, int32_t, ERROR_AXIS_DIRECTION, value >= ERROR_AXIS_DIRECTION && value <= NEG_Z)
DEFINE_CONFIGURATION_VARIABLE(BOARD_NAME, ConfigurationString<100>, "UNKNOWN_BOARD", strlen(value.str) < 100);
DEFINE_CONFIGURATION_VARIABLE(RADIO_FREQUENCY, float, 915.0, value > 0)
DEFINE_CONFIGURATION_VARIABLE(MAIN_ELEVATION, float, 200.0, value > 50)
DEFINE_CONFIGURATION_VARIABLE(DROGUE_DELAY, uint32_t, 1000, value < 60 * Units::S_TO_MS)
DEFINE_CONFIGURATION_VARIABLE(PYRO_FIRE_DURATION, uint32_t, 1000, value >= 10 && value <= 60 * Units::S_TO_MS)
DEFINE_CONFIGURATION_VARIABLE(BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR, float, 1.0, true)
DEFINE_CONFIGURATION_VARIABLE(TEST_STRUCT, Vector3D_s, Vector3D_s({1.0, 2.0, 3.0}), value.x >= 1)
DEFINE_CONFIGURATION_VARIABLE(TEST_STRING, ConfigurationString<100>, "DEFAULT", strcmp(value.str, "DEFAULT") == 0 || value.str[0] == 'P')
// clang-format on

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATIONREGISTRY_H
